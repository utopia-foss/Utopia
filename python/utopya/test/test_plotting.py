"""Test the plotting module"""

import pytest

from utopya import Multiverse

# Local constants


# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_dummy_plotting(tmpdir):
    """Test plotting of the dummy model"""
    # Create and run simulation
    mv = Multiverse(model_name='dummy',
                    update_meta_cfg=dict(paths=dict(out_dir=tmpdir)))
    mv.run_single()

    # Load
    mv.dm.load_from_cfg(print_tree=True)

    # Plot
    mv.pm.plot_from_cfg()