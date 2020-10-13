#include "homekit_accessories.h"

#include "Configuration.h"

char accessory_tarr_clean(void* ptr) {
  delete (accessory*)ptr;
  return 1;
}

void homekit_accessories::add_accessory_to_array(accessory* accessory) {
  if (!accessory) return;

  safe_array_lock(arr);
  safe_array_add(arr, accessory);
  safe_array_unlock(arr);
}

homekit_accessories::homekit_accessories() {
  arr = safe_array_init();
  add_accessory_to_array(add_accessory_bridge());
}

homekit_accessories::~homekit_accessories() {
  safe_array_clean(arr, accessory_tarr_clean);
  safe_array_free(arr);
}

service* homekit_accessories::get_accessory_information_service(
    _supla_int64_t aid, std::string firmware_revision, std::string manufacturer,
    std::string model, std::string name, std::string serial_number,
    identify_callback_routine identity_callback_routine) {
  service* result = new service(1, serviceType_accessoryInfo);

  characteristic* c_firmware_revision = new stringCharacteristic(
      aid, 2, charType_firmwareRevision, permission_read | permission_notify,
      firmware_revision);
  if (identity_callback_routine) {
    characteristic* c_identify =
        new identityCharacteristic(aid, 3, charType_identify, permission_write,
                                   identity_callback_routine, false);
    result->add_characteristic(c_identify);
  }

  characteristic* c_manufacturer = new stringCharacteristic(
      aid, 4, charType_manufactuer, permission_read, manufacturer);

  characteristic* c_model = new stringCharacteristic(aid, 5, charType_modelName,
                                                     permission_read, model);
  characteristic* c_name = new stringCharacteristic(
      aid, 6, charType_serviceName, permission_read, name);
  characteristic* c_serial_number = new stringCharacteristic(
      aid, 7, charType_serialNumber, permission_read, serial_number);

  result->add_characteristic(c_firmware_revision);
  result->add_characteristic(c_manufacturer);
  result->add_characteristic(c_model);
  result->add_characteristic(c_name);
  result->add_characteristic(c_serial_number);

  return result;
}

accessory* homekit_accessories::new_accessory(int accessoryId,
                                              service* information_service) {
  accessory* result = new accessory(accessoryId);
  result->add_service(information_service);
  return result;
}

jsoncons::ojson homekit_accessories::describe_characteristics(void) {
  safe_array_lock(arr);

  jsoncons::json characteristics = jsoncons::json::make_array();

  for (int idx = 0; idx < safe_array_count(arr); idx++) {
    accessory* a = (accessory*)safe_array_get(arr, idx);

    a->describe_characteristics(characteristics);
  }

  safe_array_unlock(arr);

  return characteristics;
}

jsoncons::ojson homekit_accessories::describe(void) {
  safe_array_lock(arr);

  jsoncons::json accessories = jsoncons::json::make_array();

  for (int a = 0; a < safe_array_count(arr); a++) {
    accessory* c = static_cast<accessory*>(safe_array_get(arr, a));
    if (c) {
      accessories.push_back(c->describe());
    }
  };

  jsoncons::json result;
  result["accessories"] = accessories;

  safe_array_unlock(arr);

  return result;
}

accessory* homekit_accessories::getAccessoryById(_supla_int64_t id) {
  safe_array_lock(arr);

  for (int a = 0; a < safe_array_count(arr); a++) {
    accessory* c = static_cast<accessory*>(safe_array_get(arr, a));
    if (c && c->getId() == id) {
      safe_array_unlock(arr);
      return c;
    }
  }

  safe_array_unlock(arr);

  return NULL;
}

void homekit_accessories::add_accessory_for_supla_channel(
    client_device_channel* channel, identify_callback_routine identify_callback,
    set_value_callback_routine value_callback) {
  int accessoryId = channel->getAccessoryId();

  service* info = get_accessory_information_service(
      channel->getAccessoryId(), channel->getFirmwareRevision(),
      channel->getManufacturer(), channel->getModel(), channel->getName(),
      channel->getSerialNumber(), identify_callback);

  int count = safe_array_count(arr);

  switch (channel->getFunc()) {
    case SUPLA_CHANNELFNC_THERMOMETER: {
      accessory* accessory = new_accessory(accessoryId, info);
      add_accessory_to_array(add_accessory_thermometer(accessory));
    } break;
    case SUPLA_CHANNELFNC_HUMIDITY: {
      accessory* accessory = new_accessory(channel->getAccessoryId(), info);
      add_accessory_to_array(add_accessory_humidity(accessory));
    } break;
    case SUPLA_CHANNELFNC_HUMIDITYANDTEMPERATURE: {
      accessory* accessory = new_accessory(accessoryId, info);
      add_accessory_thermometer(accessory);
      add_accessory_humidity(accessory);
      add_accessory_to_array(accessory);
    } break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGATEWAYLOCK: {
      //		add_accessory_to_array(add_accessory_gateway_lock(accessoryId,
      // info));

    } break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGATE: {
       //add_accessory_to_array(add_accessory_gate(accessoryId, info));

    } break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGARAGEDOOR: {
      accessory* accessory = new_accessory(accessoryId, info);
    //  add_accessory_to_array(
    //      add_accessory_garage_door(accessory, value_callback));

    } break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEDOORLOCK: {
     // accessory* accessory = new_accessory(accessoryId, info);
     // add_accessory_to_array(
     //     add_accessory_door_lock(accessory, value_callback));
    } break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER: {
      accessory* accessory = new_accessory(accessoryId, info);
      add_accessory_to_array(add_accessory_rollershutter(accessory, value_callback));
    } break;
    case SUPLA_CHANNELFNC_POWERSWITCH: {
      accessory* accessory = new_accessory(accessoryId, info);
      add_accessory_to_array(
          add_accessory_power_switch(accessory, value_callback));
    } break;
    case SUPLA_CHANNELFNC_LIGHTSWITCH: {
      accessory* accessory = new_accessory(accessoryId, info);
      add_accessory_to_array(
          add_accessory_light_switch(accessory, value_callback));
    } break;
    case SUPLA_CHANNELFNC_DIMMER: {
      // add_accessory_to_array(add_accessory_dimmer(channel->getSubId(0),
      // info));
    } break;
    case SUPLA_CHANNELFNC_RGBLIGHTING: {
      // add_accessory_to_array(add_accessory_rgb_lightning(channel->getSubId(0),
      // info));
    } break;
    case SUPLA_CHANNELFNC_DIMMERANDRGBLIGHTING: {
      // add_accessory_to_array(add_accessory_dimmer_and_rgb_lightning(channel->getSubId(0),
      // info));

    } break;
    case SUPLA_CHANNELFNC_THERMOSTAT: {
      // add_accessory_to_array(add_accessory_thermostat(channel->getSubId(0),
      // info));
    } break;

    case SUPLA_CHANNELFNC_OPENINGSENSOR_GATEWAY:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_GATE:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_GARAGEDOOR:
    case SUPLA_CHANNELFNC_NOLIQUIDSENSOR:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_ROLLERSHUTTER:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_WINDOW:
    case SUPLA_CHANNELFNC_MAILSENSOR: {
      // add_accessory_to_array(add_accessory_sensornonc(channel->getSubId(0),
      // info));
    } break;
  }

  if (safe_array_count(arr) == count) delete (info);

  if (hasController()) updateConfiguration();
}

accessory* homekit_accessories::add_accessory_gateway_lock(
    int accessoryId, service* info) /* elektrozaczep -> switch ? */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_gate(
    int accessoryId, service* info) /* brama wjazdowa -> garage door opener */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_garage_door(
    accessory* accessory,
    set_value_callback_routine
        callback) /* drzwi garażowe -> garage_door_opener */
{
  service* garage_door_opener =
      new service(accessory->getNextUUID(), serviceType_garageDoorOpener);
  garage_door_opener->setPrimary(true);

  uint8Characteristic* current_door_state = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_currentDoorState,
      permission_read | permission_notify, 0, 4, 1, unit_none, 0);
  current_door_state->setValue(0);

  current_door_state->setValidValue(0);
  current_door_state->setValidValue(1);

  uint8Characteristic* target_door_state = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_targetDoorState,
      permission_read | permission_write | permission_notify, 0, 1, 1,
      unit_none, 0);
  target_door_state->setValue(0);
  target_door_state->setCallback(callback);

  boolCharacteristic* obstruction_detected = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_obstruction,
      permission_read | permission_notify, false);

  obstruction_detected->setValue(false);

  garage_door_opener->add_characteristic(current_door_state);
  garage_door_opener->add_characteristic(target_door_state);
  garage_door_opener->add_characteristic(obstruction_detected);

  accessory->add_service(garage_door_opener);

  return accessory;
}
accessory* homekit_accessories::add_accessory_thermometer(
    accessory* accessory) /* temperatur sensor */
{
  service* temperature_sensor =
      new service(accessory->getNextUUID(), serviceType_temperatureSensor);

  floatCharacteristic* current_temperature = new floatCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_currentTemperature,
      permission_read | permission_notify, -50.0, 275.0, 0.01, unit_celsius);

  boolCharacteristic* active = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_sensorActive,
      permission_read | permission_notify, true);
  active->setValue(true);

  temperature_sensor->add_characteristic(current_temperature);
  temperature_sensor->add_characteristic(active);

  accessory->add_service(temperature_sensor);

  return accessory;
}
accessory* homekit_accessories::add_accessory_humidity(
    accessory* accessory) /* humidity sensor */
{
  service* humidity_sensor =
      new service(accessory->getNextUUID(), serviceType_humiditySensor);

  floatCharacteristic* current_relative_humidity = new floatCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_currentHumidity,
      permission_read | permission_notify, 0.00, 100.00, 0.1, unit_percentage);

  boolCharacteristic* active = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_sensorActive,
      permission_read | permission_notify, true);

  humidity_sensor->add_characteristic(active);
  humidity_sensor->add_characteristic(current_relative_humidity);

  accessory->add_service(humidity_sensor);

  return accessory;
}
accessory* homekit_accessories::add_accessory_door_lock(
    accessory* accessory,
    set_value_callback_routine
        callback) /* zamek w drzwiach  -> lock mechanizm */
{
  service* lock_mechanizm =
      new service(accessory->getNextUUID(), serviceType_lockMechanism);

  uint8Characteristic* current_door_state = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_lockCurrentState,
      permission_read | permission_notify, 0, 3, 1, unit_none, 4);

  current_door_state->setValue(4);

  current_door_state->setValidValue(0);
  current_door_state->setValidValue(1);
  current_door_state->setValidValue(2);
  current_door_state->setValidValue(3);

  uint8Characteristic* target_door_state = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_lockTargetState,
      permission_read | permission_write | permission_notify, 0, 1, 1,
      unit_none, 0);
  target_door_state->setValue(0);

  target_door_state->setCallback(callback);

  lock_mechanizm->add_characteristic(current_door_state);
  lock_mechanizm->add_characteristic(target_door_state);
  accessory->add_service(lock_mechanizm);

  return accessory;
}

accessory* homekit_accessories::add_accessory_rollershutter(
    accessory* accessory,
    set_value_callback_routine callback) /* window covering */
{
  service* window_covering =
      new service(accessory->getNextUUID(), serviceType_windowCover);

  uint8Characteristic* target_position = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_targetPosition,
      permission_read | permission_write | permission_notify, 0, 100, 1,
      unit_percentage, 0);

  target_position->setValue(0);

  target_position->setCallback(callback);

  uint8Characteristic* current_position = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_currentPosition,
      permission_read | permission_notify, 0, 100, 1, unit_percentage, 0);

  current_position->setValue(0);

  uint8Characteristic* position_state = new uint8Characteristic(
      accessory->getId(), accessory->getNextUUID(), charType_positionState,
      permission_read | permission_notify, 0, 2, 1, unit_none, 0);

  position_state->setValidValue(0);
  position_state->setValidValue(1);
  position_state->setValidValue(2);

  position_state->setValue(2);

  boolCharacteristic* hold_position =
      new boolCharacteristic(accessory->getId(), accessory->getNextUUID(),
                             charType_holdPosition, permission_write, false);
  hold_position->setCallback(callback);

  boolCharacteristic* obstruction_detected = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_obstruction,
      permission_read | permission_notify, false);

  obstruction_detected->setValue(false);

  window_covering->add_characteristic(target_position);
  window_covering->add_characteristic(current_position);
  window_covering->add_characteristic(position_state);
  window_covering->add_characteristic(hold_position);
  window_covering->add_characteristic(obstruction_detected);

  accessory->add_service(window_covering);

  return accessory;
}
accessory* homekit_accessories::add_accessory_power_switch(
    accessory* accessory, set_value_callback_routine callback) /* switch */
{
  service* light_bulb =
      new service(accessory->getNextUUID(), serviceType_switch);

  boolCharacteristic* on = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_on,
      permission_write | permission_read | permission_notify, false);

  on->setCallback(callback);

  light_bulb->add_characteristic(on);
  accessory->add_service(light_bulb);

  return accessory;
}
accessory* homekit_accessories::add_accessory_light_switch(
    accessory* accessory, set_value_callback_routine callback) /* lightswitch */
{
  service* light_bulb =
      new service(accessory->getNextUUID(), serviceType_lightBulb);

  boolCharacteristic* on = new boolCharacteristic(
      accessory->getId(), accessory->getNextUUID(), charType_on,
      permission_write | permission_read | permission_notify, false);

  on->setCallback(callback);

  light_bulb->add_characteristic(on);
  accessory->add_service(light_bulb);

  return accessory;
}
accessory* homekit_accessories::add_accessory_dimmer(
    int accessoryId, service* info) /* lightswitch */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_rgb_lightning(
    int accessoryId, service* info) /* lightswitch */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_dimmer_and_rgb_lightning(
    int accessoryId, service* info) /* lightswitch */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_sensornonc(
    int accessoryId, service* info) /* occupancy sensor */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_thermostat(
    int accessoryId, service* info) /* thermostat */
{
  return NULL;
}
accessory* homekit_accessories::add_accessory_bridge() {
  service* info = get_accessory_information_service(
      1, "1.0", "SUPLA.ORG", "BRIDGE",
      Configuration::Instance().getDeviceName(), "0000000000", NULL);
  accessory* accessory = new_accessory(1, info);

  service* protocol =
      new service(accessory->getNextUUID(), serviceType_protocolInformation);

  stringCharacteristic* version =
      new stringCharacteristic(accessory->getId(), accessory->getNextUUID(),
                               charType_version, permission_read, "1.1.0");
  version->setValue("1.1.0");

  protocol->add_characteristic(version);

  accessory->add_service(protocol);
  return accessory;
}
