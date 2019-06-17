"""Utility functions that work on the already initialized model registry."""

import logging
from typing import Union, Tuple

from . import MODELS
from .info_bundle import ModelInfoBundle
from .._yaml import load_yml

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def get_info_bundle(*, model_name: str=None, info_bundle: ModelInfoBundle=None,
                    bundle_key: Union[str, int]=None) -> ModelInfoBundle:
    """Determine the model info bundle in cases where both a model name
    and an info bundle are allowed as arguments.
    
    Args:
        model_name (str, optional): The model name.
        info_bundle (ModelInfoBundle, optional): The info bundle object.
            If given, will directly return this object again.
        bundle_key (Union[str, int], optional): In cases where only the model
            name is given, the bundle_key can be used for item access, e.g. in
            cases where more than one bundle is available and access would be
            ambiguous.
    
    Returns:
        ModelInfoBundle: The selected info bundle item
    
    Raises:
        ValueError: If neither or both model_name and info_bundle were None
    """
    if model_name is None == info_bundle is None:  # XOR-check existence
        raise ValueError("Need either of the arguments model_name or "
                         "info_bundle, but got both or neither!")

    if info_bundle:
        return info_bundle

    if bundle_key is None:
        return MODELS[model_name].item()
    return MODELS[model_name][bundle_key]

def load_model_cfg(**get_info_bundle_kwargs) -> Tuple[dict, str]:
    """Loads the default model configuration file for the given model name,
    using the path specified in the info bundle.
    
    Args:
        **get_info_bundle_kwargs: Passed on to get_info_bundle
    
    Returns:
        Tuple[dict, str]: The corresponding model configuration and the path
            to the model configuration file.
    
    Raises:
        FileNotFoundError: On missing file
        ValueError: On missing model
    
    Deleted Parameters:
        model_name (str): The name of the model to load
        info_bundle (ModelInfoBundle, optional): The info bundle the
            information is available in. If it is not given, will try to
            extract it from model registry.
        bundle_key (Union[str, int]): If the model registry entry is ambiguous,
            this parameter can help disambiguate.
    """
    bundle = get_info_bundle(**get_info_bundle_kwargs)
    path = bundle.paths['default_cfg']

    log.debug("Loading default model configuration for '%s' model ...\n  %s",
              bundle.model_name, path)

    try:
        return load_yml(path), path

    except FileNotFoundError as err:
        raise FileNotFoundError("Could not locate default configuration for "
                                "'{}' model! Expected to find it at: {}"
                                "".format(bundle.model_name, path)) from err