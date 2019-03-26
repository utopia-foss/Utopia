"""Tests of the dynamics of the ForestFire"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("ForestFire", test_file=__file__)

# Tests -----------------------------------------------------------------------

def test_dynamics_two_state_model(): 
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_dynamics_two_state.yml")

    # For the universe with f=0, ignited bottom and two_state_model=true
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['state']

        # Need the number of cells to calculate the density
        num_cells = data.shape[1] * data.shape[2]
        
        # all cells tree
        density = np.sum(data[0])/num_cells
        assert density==1.0

        # all cells burned + 1% growth
        density = np.sum(data[1,:,:])/num_cells
        assert 0 <= density <= 0.01

        # 1% growth
        density = np.sum(data[2,:,:])/num_cells
        assert 0.01 <= density <= 0.01 + 0.05


def test_percolation_mode():
    """Runs the model with the bottom row constantly ignited"""
    mv, dm = mtc.create_run_load(from_cfg="percolation_mode.yml")

    # Make sure the bottom row is always empty
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['state']

        # All cells in the bottom row are always in state empty
        assert np.all((data[1:, :, :] == 0)[:, 0, :])
        # NOTE For the initial state, this is not true.
        # NOTE The "bottom" row actually corresponds to "x" coordinate here
