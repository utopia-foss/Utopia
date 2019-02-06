"""Test the datagroup module"""

from pkg_resources import resource_filename

import pytest
import networkx as nx

import utopya
from utopya.datagroup import NetworkGroup, UniverseGroup
from utopya.testtools import ModelTest

# Local constants
NWG_CFG_PATH = resource_filename('test', 'cfg/nwg_cfg.yml')

# -----------------------------------------------------------------------------

def test_networkgroup():
    """Test the dantro.NetworkGroup integration into utopya"""
    mt = ModelTest("Hierarnet", test_file=__file__)
    mv, dm = mt.create_run_load(from_cfg=NWG_CFG_PATH)

    for uni in dm['multiverse'].values():
        nwg = uni['data/Hierarnet/nw']
        cfg = uni['cfg']

        # Check that it was loaded correctly. This requires that, on C++ side,
        # the correct attributes were set and the model was built.
        assert isinstance(nwg, NetworkGroup)

        # Test that the graph can be created as desired
        g = nwg.create_graph()

        # Check that the number of vertices matches
        assert g.number_of_nodes() == cfg['Hierarnet']['num_vertices']

def test_universegroup_get_times_array():
    """Test the universe group"""
    mt = ModelTest("dummy", test_file=__file__)
    mv, dm = mt.create_run_load(parameter_space=dict(num_steps=10))
    
    for uni in dm['multiverse'].values():
        assert isinstance(uni, UniverseGroup)

        # Check that creation of the times array works and has expected length
        times = uni.get_times_array()
        assert len(uni.get_times_array(include_t0=True)) == 1 + 10
        assert len(uni.get_times_array(include_t0=False)) == 10

        # Without configuration, it should fail
        del uni['cfg']
        with pytest.raises(ValueError, match="No configuration associated"):
            uni.get_times_array()
    
    # Again with write_every
    mv, dm = mt.create_run_load(parameter_space=dict(num_steps=10,
                                                     write_every=3))
    for uni in dm['multiverse'].values():
        # Check that creation of the times array works and has expected length
        times = uni.get_times_array()
        assert len(uni.get_times_array(include_t0=True)) == 1 + 3  # 0 3 6 9
        assert len(uni.get_times_array(include_t0=False)) == 3     #   3 6 9