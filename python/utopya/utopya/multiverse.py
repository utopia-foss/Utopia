"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import copy
import logging
from shutil import copyfile
from pkg_resources import resource_filename

import paramspace as psp

from utopya.datamanager import DataManager
from utopya.workermanager import WorkerManager
from utopya.task import enqueue_json
from utopya.reporter import WorkerManagerReporter
from utopya.tools import recursive_update, read_yml, write_yml, load_model_cfg
from utopya.info import MODELS

# Configure and get logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class Multiverse:
    """The Multiverse is where everything is orchestrated.
    
    It aims to make all the functionality of the Utopia frontend accessible in
    one place.
    
    Attributes:
        BASE_CFG_PATH (str): The path to the base configuration, supplied with
            the utopya package
        meta_cfg (dict): The parsed Multiverse meta-configuration. All
            further arguments are extracted from this dict.
        model_name (str): The model name associated with this Multiverse
        USER_CFG_SEARCH_PATH (str): The path at which a user config is expected
    """

    BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')
    USER_CFG_SEARCH_PATH = os.path.expanduser("~/.config/utopia/user_cfg.yml")

    def __init__(self, *, model_name: str, run_cfg_path: str=None, user_cfg_path: str=None, **update_meta_cfg):
        """Initialize the Multiverse.
        
        Args:
            model_name (str): A valid name of Utopia model
            run_cfg_path (str, optional): The path to the run configuration.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
            **update_meta_cfg: Can be used to update the meta configuration
                generated from 
        """
        # Initialize empty attributes (partly property-managed)
        self._meta_cfg = None
        self._model_name = None
        self._dirs = {}

        # Set the model name
        self.model_name = model_name
        
        log.info("Initializing Multiverse for '%s' model ...", self.model_name)

        # Save the model binary path and the configuration file
        self._model_binpath = MODELS[self.model_name]['binpath']
        log.debug("Associated executable of model '%s':\n  %s",
                  self.model_name, self.model_binpath)

        # Create meta configuration and list of used config files
        files = self._create_meta_cfg(run_cfg_path=run_cfg_path,
                                      user_cfg_path=user_cfg_path,
                                      update_meta_cfg=update_meta_cfg)
        # NOTE this already stores it in self._meta_cfg

        # Create the run directory and write the meta configuration into it
        self._create_run_dir(**self.meta_cfg['paths'], cfg_parts=files)

        # Provide some information
        log.info("  Run directory:  %s", self.dirs['run'])

        # Create a data manager
        self._dm = DataManager(self.dirs['run'],
                               name=self.model_name + "_data",
                               **self.meta_cfg['data_manager'])

        # Create a WorkerManager instance and pass the reporter to it
        self._wm = WorkerManager(**self.meta_cfg['worker_manager'])

        # Instantiate the Reporter
        self._reporter = WorkerManagerReporter(self._wm,
                                               report_dir=self.dirs['run'],
                                               **self.meta_cfg['reporter'])

        log.info("Initialized Multiverse.")

    # Properties ..............................................................

    @property
    def model_name(self) -> str:
        """The model name associated with this Multiverse"""
        return self._model_name

    @model_name.setter
    def model_name(self, model_name: str):
        """Checks if the model name is valid, then sets it and makes it read-only."""
        if model_name not in MODELS:
            raise ValueError("No such model '{}' available.\n"
                             "Available models: {}"
                             "".format(model_name, ", ".join(MODELS.keys())))
        
        elif self.model_name:
            raise RuntimeError("A Multiverse's associated model cannot be "
                               "changed!")

        self._model_name = model_name
        log.debug("Set model_name:  %s", model_name)

    @property
    def model_binpath(self) -> str:
        """The path to this model's binary"""
        return self._model_binpath

    @property
    def meta_cfg(self) -> dict:
        """The meta configuration."""
        return self._meta_cfg

    @property
    def dirs(self) -> dict:
        """Information on managed directories."""
        return self._dirs

    @property
    def dm(self) -> DataManager:
        """The Multiverse's DataManager."""
        return self._dm

    @property
    def wm(self) -> WorkerManager:
        """The Multiverse's WorkerManager."""
        return self._wm

    # Public methods ..........................................................

    def run(self):
        """Starts a Utopia run. Whether this will be a single simulation or
        a parameter sweep is decided by the contents of the meta config.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """
        log.info("Preparing to run Multiverse ...")

        # Depending on the configuration, the corresponding methods can already be called.
        if self.meta_cfg.get('perform_sweep'):
            self.run_sweep()
        else:
            self.run_single()
    
    def run_single(self):
        """Runs a single simulation.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """

        # Get the parameter space from the config
        pspace = self.meta_cfg['parameter_space']
        
        # If this is a ParamSpace, we need to retrieve the default point
        if isinstance(pspace, psp.ParamSpace):
            log.info("Got a ParamSpace object. Retrieving default point ...")
            uni_cfg = pspace.default

        else:
            uni_cfg = copy.deepcopy(pspace)

        # Add the task to the worker manager.
        log.info("Adding task for simulation of a single universe ...")
        self._add_sim_task(uni_id=0, max_uni_id=0, uni_cfg=uni_cfg)

        # Prevent adding further tasks to disallow further runs
        self.wm.tasks.lock()

        # Tell the WorkerManager to start working (is a blocking call)
        self.wm.start_working(**self.meta_cfg['run_kwargs'])

        log.info("Finished single universe run. Yay. :)")

    def run_sweep(self):
        """Runs a parameter sweep.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """
        
        # Get the parameter space from the config
        pspace = self.meta_cfg['parameter_space']

        if pspace.volume < 1:
            raise ValueError("The parameter space has no sweeps configured! "
                             "Refusing to run a sweep. You can either call "
                             "the run_single method or add sweeps to your "
                             "run configuration using the !sweep YAML tags.")

        log.info("Adding tasks for simulation of %d universes ...",
                 pspace.volume)

        max_uni_id = pspace.volume - 1

        for uni_cfg, uni_id in pspace.all_points(with_info=('state_no',)):
            self._add_sim_task(uni_id=uni_id, max_uni_id=max_uni_id,
                               uni_cfg=uni_cfg)
        log.info("Tasks added.")

        # Prevent adding further tasks to disallow further runs
        self.wm.tasks.lock()

        # Tell the WorkerManager to start working (is a blocking call)
        self.wm.start_working(**self.meta_cfg['run_kwargs'])

        log.info("Finished Multiverse parameter sweep. Wohoo. :)")

    # "Private" methods .......................................................

    def _create_meta_cfg(self, *, run_cfg_path: str, user_cfg_path: str, update_meta_cfg: dict) -> dict:
        """Create the meta configuration from several parts and store it.
        
        The final configuration dict is built from up to five components,
        where one is always recursively updating the previous level:
            1. base: the default configuration, which is always present
            2. user (optional): configuration of user- and machine-related
               parameters
            3. model: the model configuration, depending on self.model_name
            4. run (optional): the configuration for the current Multiverse
               instance
            5. update: if given, this dict can be used for a last update step

        The resulting configuration is the meta configuration and is stored
        to the `meta_cfg` attribute.

        The parts are recorded in the `cfg_parts` dict and returned.
        
        Args:
            run_cfg_path (str): path to run_config
            user_cfg_path (str): path to the user_config file
            update_meta_cfg (dict): will be used to update the resulting dict
        
        Returns:
            dict: dict of the parts that were needed to create the meta config.
                The dict-key corresponds to the part name, the value is the
                payload which can be either a path to a cfg file or a dict
        """
        log.debug("Reading in configuration files ...")

        # Read in the base configuration (stored inside the package)
        base_cfg = read_yml(self.BASE_CFG_PATH,
                            error_msg="Base configuration not found!")

        # Decide whether to read in the user configuration from the default search location or use a user-passed one
        if user_cfg_path is None:
            log.debug("Looking for user configuration file in default "
                      "location, %s", self.USER_CFG_SEARCH_PATH)

            if os.path.isfile(self.USER_CFG_SEARCH_PATH):
                user_cfg_path = self.USER_CFG_SEARCH_PATH
            else:
                # No user cfg will be loaded
                log.debug("No file found at the default search location.")

        elif user_cfg_path is False:
            log.debug("Not loading the user configuration from the default "
                      "search path: %s", self.USER_CFG_SEARCH_PATH)

        if user_cfg_path:
            user_cfg = read_yml(user_cfg_path,
                                error_msg="Did not find user "
                                "configuration at the specified "
                                "path {}!".format(user_cfg_path))

            # Check that it does not contain parameter_space
            if user_cfg and 'parameter_space' in user_cfg:
                raise ValueError("There was a 'parameter_space' key found in "
                                 "the user configuration loaded from {}. You "
                                 "need to remove it.".format(user_cfg_path))

        else:
            user_cfg = None

        # Read in the configuration corresponding to the chosen model
        model_cfg, model_cfg_path = load_model_cfg(self.model_name)
        # NOTE Unlike the other configuration files, this does not attach at
        # root level of the meta configuration but parameter_space.<model_name>
        # in order to allow it to be used as the default configuration for an
        # _instance_ of that model.

        # Read in the run configuration
        if run_cfg_path:
            run_cfg = read_yml(run_cfg_path,
                               error_msg="{0} was given but run_config could "
                                         "not be found.".format(run_cfg_path))
        else:
            run_cfg = None

        # After this point it is assumed that all values are valid
        # Those keys or values will throw errors once they are needed ...

        # Now perform the recursive update steps
        # Start with the base configuration
        meta_tmp = base_cfg
        log.debug("Performing recursive updates to arrive at meta "
                  "configuration ...")

        # Update with user configuration, if given
        if user_cfg:
            log.debug("Updating with user configuration ...")
            meta_tmp = recursive_update(meta_tmp, user_cfg)

        # In order to incorporate the model config, the parameter space is
        # needed. We can already be sure that the parameter_space key exists,
        # because it is added as part of the base_cfg.
        pspace = meta_tmp['parameter_space']

        # Adjust parameter space to include model configuration
        log.debug("Updating parameter space with model configuration for "
                  "model '%s' ...", self.model_name)
        pspace[self.model_name] = model_cfg
        # NOTE this works because meta_tmp is a dict and thus mutable :)

        # On top of all of that: add the run configuration, if given
        if run_cfg:
            log.debug("Updating with run configuration ...")
            meta_tmp = recursive_update(meta_tmp, run_cfg)

        # ... and the update_meta_cfg dictionary
        if update_meta_cfg:
            log.debug("Updating with given `update_meta_cfg` dictionary ...")
            meta_tmp = recursive_update(meta_tmp,
                                        copy.deepcopy(update_meta_cfg))
            # NOTE using copy to make sure that usage of the dict will not interfere with the Multiverse's meta config
        
        # Make `parameter_space` a ParamSpace object
        pspace = meta_tmp['parameter_space']
        meta_tmp['parameter_space'] = psp.ParamSpace(pspace)
        log.debug("Converted parameter_space to ParamSpace object.")
        
        # Store it
        self._meta_cfg = meta_tmp
        log.info("Loaded meta configuration.")

        # Prepare dict to store paths for config files in (for later backup)
        log.debug("Preparing dict of config parts ...")
        cfg_parts = dict(base=self.BASE_CFG_PATH,
                         user=user_cfg_path,
                         model=model_cfg_path,
                         run=run_cfg_path,
                         update=update_meta_cfg)

        # Done.
        return cfg_parts

    def _create_run_dir(self, *, out_dir: str, model_note: str=None, backup_involved_cfg_files: bool=True, cfg_parts: dict=None) -> None:
        """Create the folder structure for the run output.
        
        This will also write the meta config to the corresponding config
        directory.
        
        The following folder tree will be created
        utopia_output/   # all utopia output should go here
            model_a/
                180301-125410_my_model_note/
                    config/
                    eval/
                    universes/
                        uni000/
                        uni001/
                        ...
            model_b/
                180301-125412_my_first_sim/
                180301-125413_my_second_sim/
        
        
        Args:
            out_dir (str): The base output directory, where all simulation data
                is stored
            model_note (str, optional): The note to add to the model
            backup_involved_cfg_files (bool, optional): If true, saves all
                involved parts of the configuration process to the config
                directory. Note: the meta configuration is always saved there!
            cfg_parts (dict, optional): The parts of the config to backup
        
        Raises:
            RuntimeError: If the simulation directory already existed. This
                should not occur, as the timestamp is unique. If it occurs,
                you either started two simulations very close to each other or 
                something is seriously wrong. Strange time zone perhaps?
        """
        # Create the folder path to the simulation directory
        # NOTE the str case ensures that out_dir is not a path-like object
        # which causes problems for python < 3.6
        log.debug("Creating path for run directory inside %s ...", out_dir)
        out_dir = os.path.expanduser(str(out_dir))
        run_dir = os.path.join(out_dir,
                               self.model_name,
                               time.strftime("%y%m%d-%H%M%S"))

        # Append a model note, if needed
        if model_note:
            run_dir += "_" + model_note

        # Inform and store to directory dict
        log.debug("Expanded user and time stamp to %s", run_dir)
        self.dirs['run'] = run_dir

        # Recursively create the whole path to the simulation directory
        try:
            os.makedirs(run_dir)

        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened and is probably due "
                               "to two simulations having been started at "
                               "almost the same time. Try to start the "
                               "simulation again.") from err

        # Make subfolders
        for subdir in ('config', 'eval', 'universes'):
            subdir_path = os.path.join(run_dir, subdir)
            os.mkdir(subdir_path)
            self.dirs[subdir] = subdir_path

        log.debug("Finished creating simulation directory. Now registered: %s",
                  self._dirs)

        # Write the meta config to the config directory.
        write_yml(self.meta_cfg,
                  path=os.path.join(self.dirs['config'], "meta_cfg.yml"))
        log.debug("Wrote meta configuration to config directory.")

        # If configured, backup the other cfg files one by one
        if backup_involved_cfg_files and cfg_parts:
            log.debug("Backing up %d config parts...", len(cfg_parts))

            for part_name, val in cfg_parts.items():
                _path = os.path.join(self.dirs['config'], part_name+"_cfg.yml")
                # Distinguish two types of payload that will be saved:
                if isinstance(val, str):
                    # Assumed to be path to a config file; copy it
                    log.debug("Copying %s config ...", part_name)
                    copyfile(val, _path)

                elif isinstance(val, dict):
                    log.debug("Dumping %s config dict ...", part_name)
                    write_yml(val, path=_path)

        log.debug("Finished creating run directory and backing up config.")

    @staticmethod
    def _create_uni_basename(*, uni_id: int, max_uni_id: int) -> str:
        """Returns the formatted universe basename, zero-padded, for usage
        in WorkerTask names and universe directory creation.
        
        Args:
            uni_id (int): ID of the universe whose folder should be created.
                Needs to be positive or zero.
            max_uni_id (int): highest ID, needed for correct zero-padding.
                Needs to be larger or equal to uni_id.
        
        Returns:
            str: The zero-padded universe basename, e.g. uni0042
        
        Raises:
            ValueError: For invalid `uni_id` or `max_uni_id` arguments 
        """

        # Check if uni_id and max_uni_id are positive
        if uni_id < 0 or uni_id > max_uni_id:
            raise ValueError("Input variables don't match prerequisites: "
                             "uni_id >= 0, max_uni_id >= uni_id. Given "
                             "arguments: uni_id {}, max_uni_id {}"
                             "".format(uni_id, max_uni_id))

        # Use a format string to create the zero-padded universe basename
        return "uni{id:>0{digits:}d}".format(id=uni_id,
                                             digits=len(str(max_uni_id)))

    def _add_sim_task(self, *, uni_id: int, max_uni_id: int, uni_cfg: dict) -> None:
        """Helper function that handles task assignment to the WorkerManager.
        
        This function creates a WorkerTask that will perform the following
        actions __once it is grabbed and worked at__:
          - Create a universe (folder) for the task (simulation)
          - Write that universe's configuration to a yaml file in that folder
          - Create the correct arguments for the call to the model binary
    
        To that end, this method gathers all necessary arguments and registers
        a WorkerTask with the WorkerManager.
        
        Args:
            uni_id (int): ID of the universe whose folder should be created
            max_uni_id (int): highest ID, needed for correct zero-padding
            uni_cfg (dict): given by ParamSpace. Defines how many simulations
                should be started
        """
        def setup_universe(*, worker_kwargs: dict, model_name: str, model_binpath: str, uni_cfg: dict, uni_basename: str) -> dict:
            """The callable that will setup everything needed for a universe.
            
            This is called before the worker process starts working on the universe.
            
            Args:
                worker_kwargs (dict): the current status of the worker_kwargs
                    dictionary; is always passed to a task setup function
                model_name (str): The name of the model
                model_binpath (str): path to the binary to execute
                uni_cfg (dict): the configuration to create a yml file from
                    which is then needed by the model
                uni_basename (str): Basename of the universe to use for folder
                    creation, i.e.: zero-padded universe number, e.g. uni0042
            
            Returns:
                dict: kwargs for the process to be run when task is grabbed by
                    Worker.
            """
            # create universe directory path using the basename
            uni_dir = os.path.join(self.dirs['universes'], uni_basename)

            # Now create the folder
            os.mkdir(uni_dir)
            log.debug("Created universe directory:\n  %s", uni_dir)

            # Generate a path to the output hdf5 file and add it to the dict
            output_path = os.path.join(uni_dir, "data.h5")
            uni_cfg['output_path'] = output_path

            # write essential part of config to file:
            uni_cfg_path = os.path.join(uni_dir, "config.yml")
            write_yml(uni_cfg, path=uni_cfg_path)

            # building args tuple for task assignment
            # assuming the binary takes as only argument the path to the config
            args = (model_binpath, uni_cfg_path)

            # Generate a new worker_kwargs dict, carrying over the given ones
            wk = dict(args=args,
                      read_stdout=True,
                      line_read_func=enqueue_json,
                      **(worker_kwargs if worker_kwargs else {}))

            # Determine whether to save the streams
            if wk.get('save_streams', True):
                # Generate a path and store in the worker kwargs
                wk['save_streams_to'] = os.path.join(uni_dir, "{name:}.log")

            return wk

        # Generate the universe basename, which will be used for the folder and the task name
        uni_basename = self._create_uni_basename(uni_id=uni_id,
                                                 max_uni_id=max_uni_id)

        # Create the dict that will be passed as arguments to setup_universe
        setup_kwargs = dict(model_name=self.model_name,
                            model_binpath=self.model_binpath,
                            uni_cfg=uni_cfg,
                            uni_basename=uni_basename)

        # Process worker_kwargs
        wk = self.meta_cfg.get('worker_kwargs')

        if wk and wk.get('forward_streams') == 'in_single_run':
            # Check for the max_uni_id as indicator for a single run
            wk['forward_streams'] = bool(max_uni_id == 0)

        # Try to add a task to the worker manager
        try:
            self.wm.add_task(name=uni_basename,
                             priority=None,
                             setup_func=setup_universe,
                             setup_kwargs=setup_kwargs,
                             worker_kwargs=wk)

        except RuntimeError as err:
            # Task list was locked, probably due to a run already having taken
            # place...
            raise RuntimeError("Could not add simulation task for universe "
                               "'{}'! Did you already perform a run with this "
                               "Multiverse?"
                               "".format(uni_basename)) from err

        log.debug("Added simulation task for universe %d.", uni_id)

