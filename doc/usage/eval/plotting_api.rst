Relevant API References
=======================

This is an excerpt of the relevant API references for the plotting framework.

.. note::

    These structures are derived from corresponding dantro structures.
    For more information, visit the `dantro documentation <https://dantro.readthedocs.io/>`_.

.. contents::
   :local:
   :depth: 2

----


:py:class:`utopya.plotting.PlotManager`
---------------------------------------

.. autoclass:: utopya.plotting.PlotManager
    :noindex:
    :members: __init__, plot_from_cfg, plot



Plot Creators
-------------

:py:class:`utopya.plotting.ExternalPlotCreator`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: utopya.plotting.ExternalPlotCreator
    :noindex:
    :members: plot, _plot


:py:class:`utopya.plotting.UniversePlotCreator`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: utopya.plotting.UniversePlotCreator
    :noindex:


:py:class:`utopya.plotting.MultiversePlotCreator`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: utopya.plotting.MultiversePlotCreator
    :noindex:
    :members: _prepare_plot_func_args


:py:func:`~utopya.plotting.is_plot_func` decorator
--------------------------------------------------

.. autodecorator:: utopya.plotting.is_plot_func



:py:class:`utopya.plotting.PlotHelper`
--------------------------------------

Basic :py:class:`~utopya.plotting.PlotHelper` interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: utopya.plotting.PlotHelper
    :members: ax, available_helpers, mark_enabled, provide_defaults, invoke_enabled, enable_animation, disable_animation, invoke_helper
    :noindex:

Full :py:class:`~utopya.plotting.PlotHelper` interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. autoclass:: utopya.plotting.PlotHelper
    :noindex:
    :members:
    :inherited-members:
    :private-members:
