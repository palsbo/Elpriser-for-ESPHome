import esphome.codegen as cg
from esphome import core
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import time, text_sensor, number, select, sensor

CONF_TIMEZONE = "timezone"

AUTO_LOAD = ["time", "text_sensor", "sntp"]
DEPENDENCIES = ["network"]  # SNTP kræver netværk

test_ns = cg.esphome_ns.namespace("elpriser")
ElpriserCmp = test_ns.class_("ELPRISER", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ElpriserCmp),
    cv.Optional(CONF_TIMEZONE, default="Europe/Copenhagen"): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_TIMEZONE in config:
        cg.add(var.set_timezone(config[CONF_TIMEZONE]))

