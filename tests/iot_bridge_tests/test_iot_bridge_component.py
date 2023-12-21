import re

import pytest

from ...thirdparty.sdv_testing_tool.sdv_testing_tool.config.config import CONFIG
from ...thirdparty.sdv_testing_tool.sdv_testing_tool.enums.marks import TC_ID, PrimaryComponent, Priority, Suite, SweLevel, Tasks
from ...thirdparty.sdv_testing_tool.sdv_testing_tool.helpers.customize_marks import SDVMarks
from ...thirdparty.sdv_testing_tool.sdv_testing_tool.helpers.helpers import get_utc_time_now
from ...thirdparty.sdv_testing_tool.sdv_testing_tool.providers.service.logger import SDVLogger
from ...tests.data.test_cases_data import ram_testdata_negative, ram_testdata_positive, temp_testdata_negative, temp_testdata_positive

LOGGER = SDVLogger.get_logger("test-IOT_BRIDGE")

@SDVMarks.links("TASK_999")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.REGRESSION,
    TC_ID.SDV_00004,
)
def test_iotb_temp_negative_999(iot_bridge_dc_iot_thing):
    pass


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.SMOKE,
    TC_ID.SDV_00003,
)
@pytest.mark.parametrize("topic, test_data", temp_testdata_positive)
def test_iotb_temp_positive(iot_bridge_dc_iot_thing, topic, test_data):
    dc_service, iot_bridge_service, iot_thing = iot_bridge_dc_iot_thing

    root = dc_service.config.source_conf["collectors"]
    topic_name_qr = root["root_query"] + "/" + topic
    topic_name_send = root["root_send"] + "/" + topic

    iot_thing.subscribe_to_topic(topic_name_qr)
    iot_thing.subscribe_to_topic(topic_name_send)
    dc_service.config.generate_dc_temp_file(test_data)

    time_utc_now = get_utc_time_now()

    msg_body = {"msg": f"trigger the '{topic_name_qr}' query"}
    iot_thing.publish_message(topic_name_qr, msg_body)

    msg = f'\[ipc server\] received packet:{{"action":"forward","payload_size":.*,"topic":"{topic_name_send}"'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.REGRESSION,
    TC_ID.SDV_00004,
)
@pytest.mark.parametrize("topic, test_data", temp_testdata_negative)
def test_iotb_temp_negative(iot_bridge_dc_iot_thing, topic, test_data):
    dc_service, iot_bridge_service, iot_thing = iot_bridge_dc_iot_thing

    root = dc_service.config.source_conf["collectors"]
    topic_name_qr = root["root_query"] + "/" + topic
    topic_name_send = root["root_send"] + "/" + topic

    iot_thing.subscribe_to_topic(topic_name_qr)
    iot_thing.subscribe_to_topic(topic_name_send)
    dc_service.config.generate_dc_temp_file(test_data)

    time_utc_now = get_utc_time_now()

    msg_body = {"msg": f"trigger the '{topic_name_qr}' query"}
    iot_thing.publish_message(topic_name_qr, msg_body)

    msg = f'\[ipc server\] received packet:{{"action":"forward","payload_size":.*,"topic":"{topic_name_send}"'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.SMOKE,
    TC_ID.SDV_00007,
)
@pytest.mark.parametrize("topic, test_data", ram_testdata_positive)
def test_iotb_ram_positive(iot_bridge_dc_iot_thing, topic, test_data):
    dc_service, iot_bridge_service, iot_thing = iot_bridge_dc_iot_thing

    root = dc_service.config.source_conf["collectors"]
    topic_name_qr = root["root_query"] + "/" + topic
    topic_name_send = root["root_send"] + "/" + topic

    iot_thing.subscribe_to_topic(topic_name_qr)
    iot_thing.subscribe_to_topic(topic_name_send)
    dc_service.config.generate_dc_ram_file(test_data)

    time_utc_now = get_utc_time_now()

    msg_body = {"msg": f"trigger the '{topic_name_qr}' query"}
    iot_thing.publish_message(topic_name_qr, msg_body)

    msg = f'\[ipc server\] received packet:{{"action":"forward","payload_size":.*,"topic":"{topic_name_send}"'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.REGRESSION,
    TC_ID.SDV_00008,
)
@pytest.mark.parametrize("topic, test_data", ram_testdata_negative)
def test_iotb_ram_negative(iot_bridge_dc_iot_thing, topic, test_data):
    dc_service, iot_bridge_service, iot_thing = iot_bridge_dc_iot_thing

    root = dc_service.config.source_conf["collectors"]
    topic_name_qr = root["root_query"] + "/" + topic
    topic_name_send = root["root_send"] + "/" + topic

    iot_thing.subscribe_to_topic(topic_name_qr)
    iot_thing.subscribe_to_topic(topic_name_send)
    dc_service.config.generate_dc_ram_file(test_data)

    time_utc_now = get_utc_time_now()

    msg_body = {"msg": f"trigger the '{topic_name_qr}' query"}
    iot_thing.publish_message(topic_name_qr, msg_body)

    msg = f'\[ipc server\] received packet:{{"action":"forward","payload_size":.*,"topic":"{topic_name_send}"'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.SMOKE,
    TC_ID.SDV_00010,
)
def test_iotb_ota_positive(iot_bridge_ota_iot_thing):
    ota_service, iot_bridge_service, iot_thing = iot_bridge_ota_iot_thing

    update_qr = ota_service.config.source_conf["update_topic"]

    time_utc_now = get_utc_time_now()

    iot_thing.subscribe_to_topic(update_qr)
    signed_url = iot_thing.get_ota_service_test_package()
    msg_to_send = {"bucketUrl": signed_url}
    iot_thing.publish_message(update_qr, msg_to_send)

    msg = "\[ipc server\] sending:"
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)

    msg = f'"action":"forward","payload_size":.*,"topic":"{update_qr}"'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)

    msg = f'"bucketUrl": "{re.escape(signed_url)}"'  # TODO: get rid of re.escape here
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)

    msg = "\[ipc server\] sent packet"
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)


@SDVMarks.links("TASK_126")
@SDVMarks.add(
    SweLevel.COMPONENT,
    Priority.P1,
    PrimaryComponent.IOT_BRIDGE,
    Suite.SMOKE,
    TC_ID.SDV_00015,
)
def test_cli_iotb_positive(iot_bridge_ota_iot_thing):
    ota_service, iot_bridge_service, iot_thing = iot_bridge_ota_iot_thing

    update_qr = ota_service.config.source_conf["update_topic"]

    time_utc_now = get_utc_time_now()

    iot_thing.subscribe_to_topic(update_qr)
    signed_url = iot_thing.get_ota_service_test_package()
    msg_to_send = {"bucketUrl": signed_url}
    iot_thing.publish_message(update_qr, msg_to_send)

    msg = f"\[iotb\] received message from iot {update_qr}"
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)

    msg = f'\[iotb\] iot message {update_qr} {{"bucketUrl": "{re.escape(signed_url)}"}}'
    assert iot_bridge_service.expects_log_message(msg, since=time_utc_now)
