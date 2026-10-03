import asyncio

from dbus_fast import BusType, Variant
from dbus_fast.aio import MessageBus
from dbus_fast.service import ServiceInterface, dbus_property, method, PropertyAccess


DISNEY_MANUFACTURER_ID = 0x0183

LOCATION_BEACON_TYPE = 0x0A
PERSONALITY_BEACON_TYPE = 0x03

AFFILIATIONS = {
    'scoundrel': 1,
    'resistance': 5,
    'first_order': 9,
    'silent': 0x0d,
}

PERSONALITY_CHIPS = {
    'r_series_default': 0x01,
    'bb_series_default': 0x02,
    'blue': 0x03,
    'gray': 0x04,
    'red': 0x05,
    'orange': 0x06,
    'purple': 0x07,
    'black': 0x08,
    'cb_23': 0x09,
    'yellow': 0x0A,
    'c1_10p': 0x0B,
    'd_o': 0x0C,
    'blue_2': 0x0D,
    'bd_1': 0x0E,
    'a_lt': 0x0F,
    'white_drum': 0x10,
}


def location_beacon_payload(
    location_id: int,
    interval: int = 0x0C,
    rssi_threshold: int = 0xA6,
    flag: int = 0x01,
) -> bytes:
    return bytes([LOCATION_BEACON_TYPE, 0x04, location_id, interval, rssi_threshold, flag])


def personality_beacon_payload(
    affiliation: str,
    chip_id: int,
    paired_with_remote: bool = False,
) -> bytes:
    affiliation_id = AFFILIATIONS[affiliation]
    status = 0x01 | (0x80 if paired_with_remote else 0x00)
    affiliation_byte = 0x80 + (affiliation_id * 2)
    return bytes([PERSONALITY_BEACON_TYPE, 0x04, 0x44, status, affiliation_byte, chip_id])


_ADV_PATH = '/com/astromech/advertisement0'


class _LEAdvertisement(ServiceInterface):
    def __init__(self, manufacturer_id: int, payload: bytes):
        super().__init__('org.bluez.LEAdvertisement1')
        self._manufacturer_id = manufacturer_id
        self._payload = bytes(payload)
        self._released = asyncio.Event()

    @method()
    def Release(self):
        print('[beacon] Advertisement released by BlueZ', flush=True)
        self._released.set()

    @dbus_property(access=PropertyAccess.READ)
    def Type(self) -> 's':
        return 'broadcast'

    @dbus_property(access=PropertyAccess.READ)
    def ManufacturerData(self) -> 'a{qv}':
        return {self._manufacturer_id: Variant('ay', self._payload)}


async def run_beacon(
    beacon_payload: bytes,
    hci_device: str = 'hci0',
    refresh_interval: int = 30,
):
    mfg_id_bytes = DISNEY_MANUFACTURER_ID.to_bytes(2, 'little')
    mfg_data = mfg_id_bytes + beacon_payload
    print(f'[beacon] Starting beacon on {hci_device}', flush=True)
    print(f'[beacon] Manufacturer data: {" ".join(f"{b:02X}" for b in mfg_data)}', flush=True)

    bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
    adv = _LEAdvertisement(DISNEY_MANUFACTURER_ID, beacon_payload)
    bus.export(_ADV_PATH, adv)

    introspection = await bus.introspect('org.bluez', f'/org/bluez/{hci_device}')
    proxy = bus.get_proxy_object('org.bluez', f'/org/bluez/{hci_device}', introspection)
    adv_manager = proxy.get_interface('org.bluez.LEAdvertisingManager1')

    await adv_manager.call_register_advertisement(_ADV_PATH, {})
    print('[beacon] Broadcasting', flush=True)

    try:
        await adv._released.wait()
        raise RuntimeError('Advertisement released by BlueZ, restarting')
    finally:
        try:
            await adv_manager.call_unregister_advertisement(_ADV_PATH)
        except Exception:
            pass
        bus.disconnect()
