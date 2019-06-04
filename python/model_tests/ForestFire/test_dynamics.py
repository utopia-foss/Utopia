"""Tests of the dynamics of the ForestFire"""

import pytest
import numpy as np

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("ForestFire", test_file=__file__)

# Tests -----------------------------------------------------------------------

def test_dynamics(): 
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="dynamics.yml")

    # For the universe with f=0, ignited bottom and two_state_model=true
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['kind']

        # Need the number of cells to calculate the density
        num_cells = data.shape[1] * data.shape[2]
        
        # all cells tree at time step 0
        density = np.sum(data[0])/num_cells
        assert density == 1.0

        # all cells burned + 1% growth
        density = data.mean(dim=['x', 'y'])
        assert 0 <= density[{'time': 1}].values <= 0.02

        # 1% growth
        density = data.mean(dim=['x', 'y'])
        assert 0.01 <= density[{'time': 2}].values <= 0.02 + 0.05

def test_immunity():
    """ """
    pass

def test_fire_source():
    """Runs the model with the bottom row constantly ignited"""
    mv, dm = mtc.create_run_load(from_cfg="fire_source.yml")

    # Make sure the bottom row is always marked as source
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['kind']

        # All cells in the bottom row are always in state "source", i.e.: 3
        assert (data.isel(y=0) == 3).all()

def test_stones():
    """Runs the model with stone clusters"""
    mv, dm = mtc.create_run_load(from_cfg="stones.yml")

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['kind']

        # There is a non-zero number of stone cells
        assert (data == 4).sum() > 0

        # ... which is constant over time
        assert ((data == 4).sum(['x', 'y']).diff('time') == 0).all()
