"""The WorkerManager class."""

import os
import queue
import warnings
import logging
import time
from datetime import datetime as dt
from typing import Union, Callable, Sequence, List, Set, Dict
from typing.io import BinaryIO

from utopya.task import WorkerTask, TaskList
from utopya.stopcond import StopCondition
from utopya.reporter import WorkerManagerReporter

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class WorkerManager:
    """The WorkerManager class manages WorkerTasks.
    
    Attributes:
        num_workers (int): The number of parallel workers
        poll_delay (float): The delay (in s) between after a poll
    """

    def __init__(self, num_workers: Union[int, str]='auto', poll_delay: float=0.05, QueueCls=queue.Queue, reporter: WorkerManagerReporter=None, rf_spec: Dict[str, Union[str, List[str]]]=None, debug_mode: bool=False):
        """Initialize the worker manager.
        
        Args:
            num_workers (Union[int, str], optional): The number of workers
                that can work in parallel. If 'auto' (default), uses
                os.cpu_count(). If below zero, deduces abs(num_workers) from the CPU count.
            poll_delay (float, optional): How long (in seconds) the delay
                between worker polls should be. For too small delays (<0.01),
                the CPU load will become significant.
            QueueCls (Class, optional): Which class to use for the Queue.
                Defaults to FiFo.
            reporter (WorkerManagerReporter, optional): The reporter associated
                with this WorkerManager, reporting on the progress.
            rf_spec (Dict[str, Union[str, List[str]]], optional): The names of
                report formats that should be invoked at different points of
                the WorkerManager's operation.
                Possible keys:
                    'while_working', 'after_work', 'after_abort',
                    'task_spawn', 'task_finished'
                All other keys are ignored.
                The values of the dict can be either strings or lists of
                strings, where the strings always refer to report formats
                registered with the WorkerManagerReporter
            debug_mode (bool, optional): If True, a WorkerTask finishing with
                a non-zero exit status will lead to the WorkerManager to exit
                the working loop. This event can be caught and handled
                accordingly.
        
        Raises:
            ValueError: For too negative `num_workers` argument
        """
        log.info("Initializing WorkerManager ...")

        # Initialize attributes, some of which are property-managed
        self._num_workers = None
        self._poll_delay = None
        self._tasks = TaskList()
        self._task_q = QueueCls()
        self._active_tasks = []
        self._reporter = None
        self._num_finished_tasks = 0
        self.debug_mode = debug_mode
        self.pending_exceptions = []

        # Hand over arguments
        self.poll_delay = poll_delay

        if num_workers == 'auto':
            self.num_workers = os.cpu_count()

        elif num_workers < 0:
            try:
                self.num_workers = os.cpu_count() + num_workers
            except ValueError as err:
                raise ValueError("Received invalid argument `num_workers` of "
                                 "value {}. If giving a negative value, note "
                                 "that it needs to sum up with the CPU count "
                                 "({}) to a positive integer."
                                 "".format(num_workers, os.cpu_count())
                                 ) from err

        else:
            self.num_workers = num_workers

        # Reporter-related
        if reporter:
            self.reporter = reporter

        self.rf_spec = dict(while_working='while_working',
                            task_spawned='while_working',
                            task_finished='while_working',
                            after_work='after_work',
                            after_abort='after_work')
        if rf_spec:
            self.rf_spec.update(rf_spec)

        # Provide some information
        log.info("  Number of available CPUs:  %d", os.cpu_count())
        log.info("  Number of workers:         %d", self.num_workers)
        if self.debug_mode:
            log.info("  WorkerManager is in debug mode.")

        # Store some profiling information
        self.times = dict(init=dt.now(), start_working=None,
                          timeout=None, end_working=None)
        # These are also accessed by the reporter

        log.info("Initialized WorkerManager.")

    # Properties ..............................................................
    @property
    def tasks(self) -> TaskList:
        """The list of all tasks."""
        return self._tasks

    @property
    def task_queue(self) -> queue.Queue:
        """The task queue."""
        return self._task_q

    @property
    def task_count(self) -> int:
        """Returns the number of tasks that this manager *ever* took care of. Careful: This is NOT the current number of tasks in the queue!"""
        return len(self.tasks)

    @property
    def num_workers(self) -> int:
        """The number of workers that can work in parallel."""
        return self._num_workers

    @num_workers.setter
    def num_workers(self, val):
        """Set the number of workers that can work in parallel."""
        if val <= 0 or not isinstance(val, int):
            raise ValueError("Need positive integer for number of workers, "
                             "got {} of value {}.".format(type(val), val))

        elif val > os.cpu_count():
            warnings.warn("Set WorkerManager to use more parallel workers ({})"
                          "than there are cpu cores ({}) on this "
                          "machine.".format(val, os.cpu_count()),
                          UserWarning)

        self._num_workers = val
        log.debug("Set number of workers to %d", self.num_workers)

    @property
    def active_tasks(self) -> List[WorkerTask]:
        """The list of currently active tasks.

        Note that this information might not be up-to-date; a process might quit just after the list has been updated.
        """
        return self._active_tasks

    @property
    def num_finished_tasks(self) -> int:
        """The number of finished tasks. Incremented whenever a task leaves
        the active_tasks list.
        """
        return self._num_finished_tasks

    @property
    def num_free_workers(self) -> int:
        """Returns the number of free workers."""
        return self.num_workers - len(self.active_tasks)

    @property
    def poll_delay(self) -> float:
        """The poll frequency in polls/second. Strictly speaking: the sleep time between two polls, which roughly equals the poll frequency."""
        return self._poll_delay

    @poll_delay.setter
    def poll_delay(self, val) -> None:
        """Set the poll frequency to a positive value."""
        if val <= 0.:
            raise ValueError("Poll delay needs to be positive, was "+str(val))
        elif val < 0.01:
            warnings.warn("Setting a poll delay of {} < 0.01s can lead to "
                          "significant CPU load. Consider choosing a higher "
                          "value.", UserWarning)
        self._poll_delay = val            

    @property
    def reporter(self) -> Union[WorkerManagerReporter, None]:
        """Returns the associated Reporter object or None, if none is set."""
        return self._reporter

    @reporter.setter
    def reporter(self, reporter):
        """Set the Reporter object for this WorkerManager.

        This includes a check for the correct type and whether the reporter was
        already set.
        """
        if not isinstance(reporter, WorkerManagerReporter):
            raise TypeError("Need a WorkerManagerReporter for reporting from "
                            "WorkerManager, got {}.".format(type(reporter)))
        elif self.reporter:
            raise RuntimeError("Already set the reporter; cannot change it.")

        self._reporter = reporter

        log.debug("Set reporter of WorkerManager.")

    # Public API ..............................................................

    def add_task(self, **task_kwargs) -> WorkerTask:
        """Adds a task to the WorkerManager.
        
        Args:
            **task_kwargs: All arguments needed for WorkerTask initialization.
                See utopya.task.WorkerTask.__init__ for all valid arguments.
        """
        # Prepare the callback functions needed by the reporter
        def task_spawned(task):
            """Invokes the task_spawned report_spec"""
            self._invoke_report('task_spawned', force=True)
        
        def task_finished(task):
            """Performs actions after a task has finished.

            - invokes the 'task_finished' report specification
            - registers the task with the reporter, which extracts information
              on the run time of the task and its exit status
            - in debug mode, performs an action upon non-zero task exit status
            """
            self._invoke_report('task_finished', force=True)

            if self.reporter is not None:
                self.reporter.register_task(task)

            # If in debug mode, perform an action upon non-zero exit status
            if self.debug_mode and task.worker_status not in [0, None]:
                # Generate an exception and add it to the list of pending ones
                self.pending_exceptions.append(WorkerTaskNonZeroExit(task))

        callbacks = dict(spawn=task_spawned,
                         finished=task_finished)

        # Generate the WorkerTask object from the given parameters
        task = WorkerTask(callbacks=callbacks, **task_kwargs)

        # Append it to the task list and put it into the task queue
        self.tasks.append(task)
        self.task_queue.put_nowait(task)

        log.debug("Task %s (uid: %s) added.", task.name, task.uid)
        return task

    def start_working(self, *, detach: bool=False, forward_streams: bool=False, timeout: float=None, stop_conditions: Sequence[StopCondition]=None, post_poll_func: Callable=None) -> None:
        """Upon call, all enqueued tasks will be worked on sequentially.
        
        Args:
            detach (bool, optional): If False (default), the WorkerManager
                will block here, as it continuously polls the workers and
                distributes tasks.
            forward_streams (bool, optional): If True, workers' streams are
                forwarded to stdout via the log module.
            timeout (float, optional): If given, the number of seconds this
                work session is allowed to take. Workers will be aborted if
                the number is exceeded. Note that this is not measured in CPU time, but the host systems wall time.
            stop_conditions (Sequence[StopCondition], optional): During the
                run these StopCondition objects will be checked
            post_poll_func (Callable, optional): If given, this is called after
                all workers have been polled. It can be used to perform custom
                actions during a the polling loop.
        
        Raises:
            NotImplementedError: for `detach` True
            ValueError: For invalid (i.e., negative) timeout value
            WorkerManagerTotalTimeout: Upon a total timeout
        """
        # Determine timeout arguments
        if timeout:
            if timeout <= 0:
                raise ValueError("Invalid value for argument `timeout`: {} -- "
                                 "needs to be positive.".format(timeout))
            
            # Already calculate the time after which a timeout would be reached
            self.times['timeout'] = time.time() + timeout

            log.debug("Set timeout time to now + %f seconds", timeout) 

        # Set the variable needed for checking; if above condition was not
        # fulfilled, this will be None
        timeout_time = self.times['timeout']

        # Determine whether to detach the whole working loop
        if detach:
            # TODO implement the content of this in a separate thread.
            raise NotImplementedError("It is currently not possible to "
                                      "detach the WorkerManager from the "
                                      "main thread.")
        
        # Count the polls and save the time of the start of work
        poll_no = 0

        self.times['start_working'] = dt.now()

        log.info("Starting to work ...")
        log.debug("  Timeout:          now + %ss", timeout)
        log.debug("  Stop conditions:  %s", stop_conditions)

        # Enter the polling loop, where most of the time will be spent
        
        # Start with the polling loop
        # Basically all working time will be spent in there ...
        try:
            while self.active_tasks or self.task_queue.qsize() > 0:
                # Check total timeout
                if timeout_time is not None and time.time() > timeout_time:
                    raise WorkerManagerTotalTimeout()

                # Check if there was another reason for exiting
                self._handle_pending_exceptions()

                # Check if there are free workers
                if self.num_free_workers:
                    # Yes. => Try to grab a task and start working on it
                    try:
                        new_task = self._grab_task()
                    except queue.Empty:
                        # There were no tasks left in the task queue
                        pass
                    else:
                        # Succeeded in grabbing a task; worker spawned
                        self.active_tasks.append(new_task)
                    # NOTE Only a single task is grabbed here, even if there is
                    # more than one free worker. This is to assure that the
                    # while loop iterations have comparable run time, even if
                    # a task is added (which can take O(ms)). As this loop
                    # handles more than just grabbing new tasks, it is safer
                    # to have it run reliably and foreseeably.
                    # Also, the poll delay is usually not so large that there
                    # would be an issue with workers staying idle for too long.

                # Invoke the reporter, if available
                self._invoke_report('while_working')

                # Gather the streams of all working workers
                for task in self.active_tasks:
                    task.read_streams(forward_streams=forward_streams)

                # Check stop conditions
                if stop_conditions is not None:
                    # Compile a list of workers that need to be terminated
                    to_terminate = self._check_stop_conds(stop_conditions)
                    self._signal_workers(to_terminate, signal='SIGTERM')

                # Poll the workers
                self._poll_workers()
                # NOTE this will also remove no longer active workers

                # Call the post-poll function
                if post_poll_func is not None:
                    log.debug("Calling post_poll_func %s ...",
                              post_poll_func.__name__)
                    post_poll_func()

                # Some information
                poll_no += 1
                log.debug("Poll # %6d:  %d active tasks",
                          poll_no, len(self.active_tasks))

                # Delay the next poll
                time.sleep(self.poll_delay)

            # Finished working

            # Still handle exceptions
            self._handle_pending_exceptions()
            # TODO set `handle_all` once implemented

        except WorkerManagerError as err:
            print("")
            log.warning("Did not finish working due to a %s ...",
                        err.__class__.__name__)
            
            log.warning("Terminating active tasks ...")
            self._signal_workers(self.active_tasks, signal='SIGTERM')

            # Store end time and invoke a report
            self.times['end_working'] = dt.now()
            self._invoke_report('after_abort', force=True)

            raise

        # Register end time and invoke final report
        self.times['end_working'] = dt.now()
        self._invoke_report('after_work', force=True)

        print("")
        log.info("Finished working. Total tasks worked on: %d",
                 self.task_count)

    # Non-public API ..........................................................

    def _invoke_report(self, rf_spec_name: str, *args, **kwargs):
        """Helper function to invoke the reporter's report function"""
        if self.reporter is not None:
            # Resolve the spec name
            rfs = self.rf_spec[rf_spec_name]
            
            if not isinstance(rfs, list):
                rfs = [rfs]

            for rf in rfs:
                self.reporter.report(rf, *args, **kwargs)

    def _grab_task(self) -> WorkerTask:
        """Will initiate that a task is gotten from the queue and that it
        spawns its worker process.
        
        Returns:
            WorkerTask: The WorkerTask grabbed from the queue.
        
        Raises:
            queue.Empty: If the task queue was empty
        """

        # Get a task from the queue
        try:
            task = self.task_queue.get_nowait()

        except queue.Empty as err:
            raise queue.Empty("No more tasks available in tasks queue.") from err
        
        else:
            log.debug("Got task %s from queue. (Priority: %s)",
                      task.uid, task.priority)

        # Let it spawn its own worker
        task.spawn_worker()

        # Now return the task
        return task
    
    def _poll_workers(self) -> None:
        """Will poll all workers that are in the working list and remove them from that list if they are no longer alive.
        """
        # Poll the task's worker's status
        for task in self.active_tasks:
            if task.worker_status is not None:
                # This task has finished. Need to rebuild the list
                break
        else:
            # Nothing to rebuild
            return

        # Broke out of the loop, i.e.: at least one task finished
        old_len = len(self.active_tasks)

        # have to rebuild the list of active tasks now...
        self.active_tasks[:] = [t for t in self.active_tasks
                                if t.worker_status is None]
        # NOTE this will also poll all other active tasks and potentially not add them to the active_tasks list again.
        # Now, only active tasks are in the list, but the list is shorter
        # Can deduce the number of finished tasks from this
        self._num_finished_tasks += (old_len - len(self.active_tasks))

        return

    def _check_stop_conds(self, stop_conds: Sequence[StopCondition]) -> Set[WorkerTask]:
        """Checks the given stop conditions for the active tasks and compiles a list of workers that need to be terminated.
        
        Args:
            stop_conds (Sequence[StopCondition]): The stop conditions that
                are to be checked.
        
        Returns:
            List[WorkerTask]: The WorkerTasks whose workers need to be
                terminated
        """
        to_terminate = []
        log.debug("Checking %d stop condition(s) ...", len(stop_conds))

        for sc in stop_conds:
            log.debug("Checking stop condition '%s' ...", sc.name)
            fulfilled = [t for t in self.active_tasks if sc.fulfilled(t)]

            if fulfilled:
                log.debug("Stop condition '%s' was fulfilled for %d task(s) "
                          "with name(s):  %s", sc.name, len(fulfilled),
                          ", ".join([t.name for t in fulfilled]))
                to_terminate += fulfilled

        # Return as set to be sure that they are unique
        return set(to_terminate)

    def _signal_workers(self, tasks: Union[str, List[WorkerTask]], *, signal: Union[str, int]) -> None:
        """Send signals to a list of WorkerTasks.
        
        Args:
            tasks (Union[str, List[WorkerTask]]): strings 'all' or 'active' or
                a list of WorkerTasks to signal
            signal (Union[str, int]): The signal to send
        """
        if isinstance(tasks, str):
            if tasks == 'all':
                tasks = self.tasks
            elif tasks == 'active':
                tasks = self.active_tasks
            else:
                raise ValueError("Tasks cannot be specified by string '{}', "
                                 "allowed strings are: 'all', 'active'."
                                 "".format(tasks))

        if not tasks:
            log.debug("No worker tasks to signal.")
            return 

        log.debug("Sending signal %s to %d task(s) ...", signal, len(tasks))
        for task in tasks:
            task.signal_worker(signal)
        
        log.debug("All tasks signalled. Tasks' worker status:\n  %s",
                  ", ".join([str(t.worker_status) for t in tasks]))

    def _handle_pending_exceptions(self, handle_all: bool=False) -> None:
        """This method handles the list of pending exceptions during working
        
        Args:
            handle_all (bool, optional): Whether to handle all pending
                exceptions. False: only handle the latest.
        
        Returns:
            None
        
        Raises:
            exc: The latest exception
            NotImplementedError: handle_all
        """

        if handle_all:
            raise NotImplementedError("handle_all")

        if not self.pending_exceptions:
            return
        # else: there was an exception

        # In debug mode, raise
        if self.debug_mode:
            # If there is a reporter, set it to suppress CRs
            if self.reporter is not None:
                self.reporter.suppress_cr = True

            # Raise the latest pending exception
            raise self.pending_exceptions.pop()
            # If this was an exception that is derived from the
            # WorkerManagerError, execution continues in the except
            # block at the end of this while loop

        # Otherwise, just log it as a warning
        log.warning(str(exc))


# Custom exceptions -----------------------------------------------------------


class WorkerManagerError(BaseException):
    """The base exception class for WorkerManager errors"""
    pass


class WorkerManagerTotalTimeout(WorkerManagerError):
    """Raised when a total timeout occured"""
    pass


class WorkerTaskError(WorkerManagerError):
    """Raised when there was an error in a WorkerTask"""
    pass


class WorkerTaskNonZeroExit(WorkerTaskError):
    """Can be raised when a WorkerTask exited with a non-zero exit code."""
    
    def __init__(self, task: WorkerTask, *args, **kwargs):
        # Store the task        
        self.task = task

        # Pass everything else to the parent init
        super().__init__(*args, **kwargs)
