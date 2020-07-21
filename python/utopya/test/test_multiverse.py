"""Test the Multiverse class initialization and workings.

As the Multiverse will always generate a folder structure, it needs to be
taken care that these output folders are temporary and are deleted after the
tests. This can be done with the tmpdir fixture of pytest.
"""

import os
import copy
import uuid
import time
from pkg_resources import resource_filename

import pytest

from utopya import Multiverse, FrozenMultiverse
from utopya.multiverse import DataManager, PlotManager, WorkerManager
from utopya.parameter import ValidationError

# Get the test resources
RUN_CFG_PATH = resource_filename('test', 'cfg/run_cfg.yml')
RUN_CFG_PATH_VALID = resource_filename('test', 'cfg/run_cfg_validation_valid.yml')
RUN_CFG_PATH_INVALID = resource_filename('test', 'cfg/run_cfg_validation_invalid.yml')
USER_CFG_PATH = resource_filename('test', 'cfg/user_cfg.yml')
BASE_PLOTS_PATH = resource_filename('test', 'cfg/base_plots.yml')
UPDATE_BASE_PLOTS_PATH = resource_filename('test', 'cfg/update_base_plots.yml')
SWEEP_CFG_PATH = resource_filename('test', 'cfg/sweep_cfg.yml')
STOP_COND_CFG_PATH = resource_filename('test','cfg/stop_conds_integration.yml')
CLUSTER_MODE_CFG_PATH = resource_filename('test', 'cfg/cluster_mode_cfg.yml')

# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation.

    This uses the `tmpdir` fixture provided by pytest, which creates a unique
    temporary directory that is removed after the tests ran through.
    """
    # Create a dict that specifies a unique testing path.
    # The str cast is needed for python version < 3.6
    rand_str = "test_" + uuid.uuid4().hex
    unique_paths = dict(out_dir=str(tmpdir), model_note=rand_str)

    return dict(model_name="dummy",
                run_cfg_path=RUN_CFG_PATH,
                user_cfg_path=USER_CFG_PATH,
                paths=unique_paths)

@pytest.fixture
def default_mv(mv_kwargs) -> Multiverse:
    """Initialises a unique default configuration of the Multiverse to test
    everything beyond initialisation.

    Using the mv_kwargs fixture, it is assured that the output directory is
    unique.
    """
    return Multiverse(**mv_kwargs)

@pytest.fixture(params=["node[002-004,011,006]", "node[002,003-004,011,006]"])
def cluster_env(tmpdir, request) -> dict:
    return dict(TEST_JOB_ID="123",
                TEST_JOB_NUM_NODES="5",
                TEST_JOB_NODELIST=request.param,
                TEST_NODENAME="node006",
                TEST_JOB_NAME="testjob",
                TEST_JOB_ACCOUNT="testaccount",
                TEST_CPUS_ON_NODE="42",
                TEST_CLUSTER_NAME="testcluster",
                TEST_TIMESTAMP=str(int(time.time())),
                TEST_CUSTOM_OUT_DIR=str(tmpdir.join("my_custom_dir"))
                )

# Initialisation tests --------------------------------------------------------

def test_simple_init(mv_kwargs):
    """Tests whether initialisation works for all basic cases."""
    # With the full set of arguments
    mv = Multiverse(**mv_kwargs)

    # Assert some basic types
    assert isinstance(mv.wm, WorkerManager)
    assert isinstance(mv.dm, DataManager)
    assert isinstance(mv.pm, PlotManager)

    # Without the run configuration
    mv_kwargs.pop('run_cfg_path')
    mv_kwargs['paths']['model_note'] += "_wo_run_cfg"
    Multiverse(**mv_kwargs)

    # Suppressing the user config
    mv_kwargs['user_cfg_path'] = False
    mv_kwargs['paths']['model_note'] += "_wo_user_cfg"
    Multiverse(**mv_kwargs)
    # NOTE Without specifying a path, the search path will be used, which makes
    # the results untestable and creates spurious folders for the user.
    # Therefore, we cannot test for the case where no user config is given ...

    # No user config path given -> search at default location
    mv_kwargs['user_cfg_path'] = None
    mv_kwargs['paths']['model_note'] = "_user_cfg_path_none"
    Multiverse(**mv_kwargs)

    # No user config at default search location
    Multiverse.USER_CFG_SEARCH_PATH = "this_is_not_a_path"
    mv_kwargs['paths']['model_note'] = "_user_cfg_path_none_and_no_class_var"
    Multiverse(**mv_kwargs)

def test_invalid_model_name_and_operation(default_mv, mv_kwargs):
    """Tests for correct behaviour upon invalid model names"""
    # Try to instantiate with invalid model name
    mv_kwargs['model_name'] = "invalid_model_RandomShit_bgsbjkbkfvwuRfopiwehGP"
    with pytest.raises(KeyError, match="No model with name 'invalid_model_"):
        Multiverse(**mv_kwargs)

def test_config_handling(mv_kwargs):
    """Tests the config handling of the Multiverse"""
    # Multiverse that does not load the default user config
    mv_kwargs['user_cfg_path'] = False
    Multiverse(**mv_kwargs)

    # Testing whether errors are raised
    # Multiverse with wrong run config
    mv_kwargs['run_cfg_path'] = 'an/invalid/run_cfg_path'
    with pytest.raises(FileNotFoundError):
        Multiverse(**mv_kwargs)

def test_backup(mv_kwargs):
    """Tests whether the backup of all config parts and the executable works"""
    mv = Multiverse(**mv_kwargs)
    cfg_path = mv.dirs['config']

    assert os.path.isfile(os.path.join(cfg_path, 'base_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'user_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'model_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'run_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'update_cfg.yml'))

    # And once more, now including the executable
    mv_kwargs['backups'] = dict(backup_executable=True)
    mv_kwargs['paths']['model_note'] = "with-exec-backup"
    mv = Multiverse(**mv_kwargs)

    assert os.path.isfile(os.path.join(mv.dirs['run'], 'backup', 'dummy'))


def test_create_run_dir(default_mv):
    """Tests the folder creation in the initialsation of the Multiverse."""
    mv = default_mv

    # Reconstruct path from meta-config to have a parameter to compare to
    out_dir = str(mv.meta_cfg['paths']['out_dir'])  # need str for python < 3.6
    path_base = os.path.join(out_dir, mv.model_name)

    # get all folders in the output dir
    folders = os.listdir(path_base)

    # select the latest one (there should be only one anyway)
    latest = folders[-1]

    # Check if the subdirectories are present
    for folder in ['config', 'data', 'eval']:
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

def test_detect_doubled_folders(mv_kwargs):
    """Tests whether an existing folder will raise an exception."""
    # Init Multiverse
    Multiverse(**mv_kwargs)

    # create output folders again
    # expect error due to existing folders
    with pytest.raises(RuntimeError, match="Simulation directory already"):
        # And another one, that will also create a directory
        Multiverse(**mv_kwargs)
        Multiverse(**mv_kwargs)
        # NOTE this test assumes that the two calls are so close to each other
        #      that the timestamp is the same, that's why there are two calls
        #      so that the latest the second call should raise such an error

def test_parameter_validation(mv_kwargs):
    """Tests integration of the parameter validation feature"""
    # Works
    mv_kwargs['run_cfg_path'] = RUN_CFG_PATH_VALID
    mv_kwargs['model_name'] = "ForestFire"
    mv_kwargs['paths']['model_note'] = "valid"
    mv = Multiverse(**mv_kwargs)
    mv.run_single()

    # Fails
    mv_kwargs['run_cfg_path'] = RUN_CFG_PATH_INVALID
    mv_kwargs['model_name'] = "ForestFire"
    mv_kwargs['paths']['model_note'] = "invalid"
    with pytest.raises(ValidationError, match="Validation failed for 3 para"):
        mv = Multiverse(**mv_kwargs)

def test_prepare_executable(mv_kwargs):
    """Tests handling of the executable, i.e. copying to a temporary location
    and emitting helpful error messages
    """
    mv_kwargs['executable_control'] = dict(run_from_tmpdir=False)
    mv = Multiverse(**mv_kwargs)

    # The dummy model should be available at this point, so _prepare_executable
    # should have correctly set a binary path, but not in a temporary directory
    assert mv._model_binpath is not None
    assert mv._tmpdir is None
    original_binpath = mv._model_binpath

    # Now, let the executable be copied to a temporary location
    mv._prepare_executable(run_from_tmpdir=True)
    assert mv._model_binpath != original_binpath
    assert mv._tmpdir is not None

    # Adjust the info bundle for this Multiverse to use the temporary location
    tmp_binpath = mv._model_binpath
    mv._info_bundle = copy.deepcopy(mv.info_bundle)
    mv._info_bundle.paths['binary'] = tmp_binpath

    # With the executable in a temporary location, we can change its access
    # rights to test the PermissionError
    os.chmod(tmp_binpath, 0o600)
    with pytest.raises(PermissionError, match="is not executable"):
        mv._prepare_executable()

    # Finally, remove that (temporary) file, to test the FileNotFound error
    os.remove(tmp_binpath)
    with pytest.raises(FileNotFoundError, match="Did you build it?"):
        mv._prepare_executable()


# Simulation tests ------------------------------------------------------------

def test_run_single(default_mv):
    """Tests a run with a single simulation"""
    # Run a single simulation using the default multiverse
    default_mv.run()
    # NOTE run will check the meta configuration for perform_sweep parameter
    #      and accordingly call run_single

    # Test that the universe directory was created as a proxy for the run
    # being finished
    assert os.path.isdir(os.path.join(default_mv.dirs['data'], 'uni0'))

    # ... and nothing else in the data directory
    assert len(os.listdir(default_mv.dirs['data'])) == 1

def test_run_sweep(mv_kwargs):
    """Tests a run with a single simulation"""
    # Adjust the defaults to use the sweep configuration for run configuration
    mv_kwargs['run_cfg_path'] = SWEEP_CFG_PATH
    mv = Multiverse(**mv_kwargs)

    # Run the sweep
    mv.run()

    # There should now be four directories in the data directory
    assert len(os.listdir(mv.dirs['data'])) == 4

    # With a parameter space without volume, i.e. without any sweeps added,
    # the sweep should not be possible
    mv_kwargs['run_cfg_path'] = RUN_CFG_PATH
    mv_kwargs['paths']['model_note'] = "_invalid_cfg"
    mv = Multiverse(**mv_kwargs)

    with pytest.raises(ValueError, match="The parameter space has no sweeps"):
        mv.run_sweep()

def test_multiple_runs_not_allowed(mv_kwargs):
    """Assert that multiple runs are prohibited"""
    # Create Multiverse and run
    mv = Multiverse(**mv_kwargs)
    mv.run_single()

    # Another run should not be possible
    with pytest.raises(RuntimeError, match="Could not add simulation task"):
        mv.run_single()

def test_stop_conditions(mv_kwargs):
    """An integration test for stop conditions"""
    mv_kwargs['run_cfg_path'] = STOP_COND_CFG_PATH
    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert len(mv.wm.tasks) == 13
    assert len(mv.wm.stopped_tasks) == 13  # all stopped

def test_renew_plot_manager(mv_kwargs):
    """Tests the renewal of PlotManager instances in the Multiverse"""
    mv = Multiverse(**mv_kwargs)
    initial_pm = mv.pm

    # Try to renew it (with a bad config). The old one should remain
    with pytest.raises(ValueError, match="Failed setting up"):
        mv.renew_plot_manager(foo="bar")

    assert mv.pm is initial_pm

    # Again, this time it should work
    mv.renew_plot_manager()
    assert mv.pm is not initial_pm

def test_cluster_mode(mv_kwargs, cluster_env):
    """Tests cluster mode basics like: resolution of parameters, creation of
    the run directory, ...
    """
    # Define a custom test environment
    mv_kwargs['run_cfg_path'] = CLUSTER_MODE_CFG_PATH
    mv_kwargs['cluster_params'] = dict(env=cluster_env)

    # Create the Multiverse
    mv = Multiverse(**mv_kwargs)

    rcps = mv.resolved_cluster_params
    assert len(rcps) == 10 + 1

    # Check the custom output directory
    assert 'my_custom_dir' in mv.dirs['run']

    # Check the job ID is part of the run directory path
    assert 'job123' in mv.dirs['run']

    # Make sure the required keys are available
    assert all([k in rcps for k in ('job_id', 'num_nodes', 'node_list',
                                    'node_name', 'timestamp')])

    # Check some types
    assert isinstance(rcps['job_id'], int)
    assert isinstance(rcps['num_nodes'], int)
    assert isinstance(rcps['num_procs'], int)
    assert isinstance(rcps['node_list'], list)
    assert isinstance(rcps['timestamp'], int)

    # Check some values
    assert rcps['node_index'] == 3  # for node006
    assert rcps['timestamp'] > 0
    assert rcps['node_list'] == ["node002", "node003", "node004",
                                 "node006", "node011"]

    # Can add additional info to the run directory
    mv_kwargs['cluster_params']['additional_run_dir_fstrs'] = ["xyz{job_id:}",
                                                               "N{num_nodes:}"]
    mv = Multiverse(**mv_kwargs)
    assert 'xyz123_N5' in mv.dirs['run']

    # Single-node case
    cluster_env['TEST_JOB_NUM_NODES'] = '1'
    cluster_env['TEST_JOB_NODELIST'] = "node006"
    mv = Multiverse(**mv_kwargs)
    assert mv.resolved_cluster_params['node_list'] == ["node006"]

    # Test error messages
    # Node name not in node list
    cluster_env['TEST_NODENAME'] = 'node042'
    with pytest.raises(ValueError, match="`node_name` 'node042' is not part"):
        Multiverse(**mv_kwargs)

    # Wrong number of nodes
    cluster_env['TEST_NODENAME'] = 'node003'
    cluster_env['TEST_JOB_NUM_NODES'] = '3'
    with pytest.raises(ValueError, match="`node_list` has a different length"):
        Multiverse(**mv_kwargs)

    # Missing environment variables
    cluster_env.pop('TEST_NODENAME')
    with pytest.raises(ValueError,
                       match="Missing required environment variable"):
        Multiverse(**mv_kwargs)

def test_cluster_mode_run(mv_kwargs, cluster_env):
    # Define a custom test environment
    mv_kwargs['run_cfg_path'] = CLUSTER_MODE_CFG_PATH
    mv_kwargs['cluster_params'] = dict(env=cluster_env)

    # Parameter space has 12 points
    # Five nodes are being used: node002, node003, node004, node006, node011
    # Test for first node, should perform 3 simulations
    cluster_env['TEST_NODENAME'] = "node002"
    mv_kwargs['paths']['model_note'] = "node002"

    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert mv.wm.num_finished_tasks == 3
    assert [t.name for t in mv.wm.tasks] == ['uni01', 'uni06', 'uni11']
    # NOTE: simulated universes are uni01 ... uni12

    # Test for second node, should also perform 3 simulations
    cluster_env['TEST_NODENAME'] = "node003"
    mv_kwargs['paths']['model_note'] = "node003"

    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert mv.wm.num_finished_tasks == 3
    assert [t.name for t in mv.wm.tasks] == ['uni02', 'uni07', 'uni12']

    # The third node should only perform 2 simulations
    cluster_env['TEST_NODENAME'] = "node004"
    mv_kwargs['paths']['model_note'] = "node004"

    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert mv.wm.num_finished_tasks == 2
    assert [t.name for t in mv.wm.tasks] == ['uni03', 'uni08']

    # The fourth and fifth node should also perform only 2 simulations
    cluster_env['TEST_NODENAME'] = "node006"
    mv_kwargs['paths']['model_note'] = "node006"

    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert mv.wm.num_finished_tasks == 2
    assert [t.name for t in mv.wm.tasks] == ['uni04', 'uni09']

    cluster_env['TEST_NODENAME'] = "node011"
    mv_kwargs['paths']['model_note'] = "node011"

    mv = Multiverse(**mv_kwargs)
    mv.run_sweep()
    assert mv.wm.num_finished_tasks == 2
    assert [t.name for t in mv.wm.tasks] == ['uni05', 'uni10']


def test_parallel_init(mv_kwargs):
    """Test enabling parallel execution through the config"""
    # NOTE Ensure that information on parallel execution is logged
    mv_kwargs['parameter_space'] = dict(log_levels=dict(core='debug'))

    # Run with default settings and check log message
    mv_kwargs['paths']['model_note'] = "pexec_disabled"
    mv = Multiverse(**mv_kwargs)
    mv.run()
    log = mv.wm.tasks[0].streams['out']['log']
    assert any(("Parallel execution disabled" in line for line in log))

    # Now override default setting
    mv_kwargs['parameter_space']['parallel_execution'] = dict(enabled=True)
    mv_kwargs['paths']['model_note'] = "pexec_enabled"
    mv = Multiverse(**mv_kwargs)
    mv.run()
    log = mv.wm.tasks[0].streams['out']['log']
    assert any(("Parallel execution enabled" in line for line in log))


# FrozenMultiverse tests ------------------------------------------------------

def test_FrozenMultiverse(mv_kwargs, cluster_env):
    """Test the FrozenMultiverse class"""
    # Need a regular Multiverse and corresponding output for that
    mv = Multiverse(**mv_kwargs)
    mv.run_single()

    # NOTE Need to adjust the data directory in order to not create collisions
    # in the eval directory due to same timestamps ...

    # Now create a frozen Multiverse from that one
    # Without run directory, the latest one should be loaded
    print("\nInitializing FrozenMultiverse without further kwargs")
    FrozenMultiverse(**mv_kwargs,
                     data_manager=dict(out_dir="eval/{timestamp:}_1"))


    # With a relative path, the corresponding directory should be found
    print("\nInitializing FrozenMultiverse with timestamp as run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=os.path.basename(mv.dirs['run']),
                     data_manager=dict(out_dir="eval/{timestamp:}_2"))

    # With an absolute path, that path should be used directly
    print("\nInitializing FrozenMultiverse with absolute path to run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=mv.dirs['run'],
                     data_manager=dict(out_dir="eval/{timestamp:}_3"))

    # With a relative path, the path relative to the CWD should be used
    print("\nInitializing FrozenMultiverse with relative path to run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=os.path.relpath(mv.dirs['run'],
                                                          start=os.getcwd()),
                     data_manager=dict(out_dir="eval/{timestamp:}_4"))

    # Bad type of run directory should fail
    with pytest.raises(TypeError, match="Argument run_dir needs"):
        FrozenMultiverse(**mv_kwargs, run_dir=123,
                         data_manager=dict(out_dir="eval/{timestamp:}_5"))

    # Non-existing directory should fail
    with pytest.raises(IOError, match="No run directory found at"):
        FrozenMultiverse(**mv_kwargs, run_dir="my_non-existing_directory",
                         data_manager=dict(out_dir="eval/{timestamp:}_6"))

    # Cluster mode
    print("\nInitializing FrozenMultiverse in cluster mode")
    mv_kwargs['run_cfg_path'] = CLUSTER_MODE_CFG_PATH
    mv_kwargs['cluster_params'] = dict(env=cluster_env)
    FrozenMultiverse(**mv_kwargs, run_dir=os.path.relpath(mv.dirs['run'],
                                                          start=os.getcwd()),
                     data_manager=dict(out_dir="eval/{timestamp:}_7"))


    with pytest.raises(NotImplementedError, match="use_meta_cfg_from_run_dir"):
        FrozenMultiverse(**mv_kwargs, run_dir="/some/path/to/a/run_dir",
                         use_meta_cfg_from_run_dir=True,
                         data_manager=dict(out_dir="eval/{timestamp:}_7"))
