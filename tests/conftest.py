import pytest

from thirdparty.sdv_testing_tool.sdv_testing_tool.config.config import CONFIG
from thirdparty.sdv_testing_tool.sdv_testing_tool.providers.service.logger import SDVLogger

LOGGER = SDVLogger.get_logger("CONFTEST")
