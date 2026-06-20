from __future__ import annotations

import asyncio
import subprocess
from typing import Optional


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


def _manufacturer_id_bytes() -> bytes:
    return DISNEY_MANUFACTURER_ID.to_bytes(2, 'little')


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


def _build_advertising_data(beacon_payload: bytes) -> bytes:
    flags = bytes([0x02, 0x01, 0x06])
    mfg_id = _manufacturer_id_bytes()
    mfg_structure = bytes([1 + len(mfg_id) + len(beacon_payload), 0xFF]) + mfg_id + beacon_payload
    ad_data = flags + mfg_structure
    return bytes([len(ad_data)]) + ad_data


def _hci_set_advertising_data(ad_data: bytes, hci_device: str = 'hci0'):
    hex_args = ' '.join(f'0x{b:02X}' for b in ad_data)
    # HCI command 0x08 0x0008 = LE Set Advertising Data
    # The command expects exactly 32 bytes of data (padded with zeros)
    pad_length = 32 - len(ad_data)
    if pad_length > 0:
        hex_args += ' ' + ' '.join(['0x00'] * pad_length)
    subprocess.run(
        ['sudo', 'hcitool', '-i', hci_device, 'cmd', '0x08', '0x0008'] + hex_args.split(),
        check=True,
    )


def _hci_start_advertising(hci_device: str = 'hci0'):
    # leadv 3 = non-connectable undirected advertising (broadcast only)
    subprocess.run(
        ['sudo', 'hciconfig', hci_device, 'leadv', '3'],
    )


def _hci_stop_advertising(hci_device: str = 'hci0'):
    subprocess.run(
        ['sudo', 'hciconfig', hci_device, 'noleadv'],
    )


def _apply_beacon(ad_data: bytes, hci_device: str = 'hci0') -> bool:
    try:
        _hci_set_advertising_data(ad_data, hci_device)
        _hci_start_advertising(hci_device)
        return True
    except Exception as e:
        print(f"[beacon] Failed to apply advertising: {e}", flush=True)
        return False


def start_beacon(beacon_payload: bytes, hci_device: str = 'hci0'):
    ad_data = _build_advertising_data(beacon_payload)
    mfg_id = _manufacturer_id_bytes()
    mfg_data = mfg_id + beacon_payload
    print(f"[beacon] Starting beacon on {hci_device}", flush=True)
    print(f"[beacon] Manufacturer ID: 0x{DISNEY_MANUFACTURER_ID:04X} (Disney)", flush=True)
    print(f"[beacon] Manufacturer data: {' '.join(f'{b:02X}' for b in mfg_data)}", flush=True)
    print(f"[beacon] Full advertising data: {' '.join(f'{b:02X}' for b in ad_data)}", flush=True)
    print(f"[beacon] On your phone scanner, look for a device with:", flush=True)
    print(f"[beacon]   - No name (non-connectable broadcast)", flush=True)
    print(f"[beacon]   - Manufacturer/Company ID: 0x{DISNEY_MANUFACTURER_ID:04X} ({DISNEY_MANUFACTURER_ID})", flush=True)
    print(f"[beacon]   - Payload: {' '.join(f'{b:02X}' for b in beacon_payload)}", flush=True)
    if _apply_beacon(ad_data, hci_device):
        print(f"[beacon] Broadcasting", flush=True)
    else:
        print(f"[beacon] Initial start failed, will retry", flush=True)


def stop_beacon(hci_device: str = 'hci0'):
    print(f"[beacon] Stopping beacon on {hci_device}", flush=True)
    _hci_stop_advertising(hci_device)


def _check_advertising(hci_device: str = 'hci0') -> bool:
    result = subprocess.run(
        ['hciconfig', hci_device],
        capture_output=True, text=True,
    )
    return 'UP RUNNING' in result.stdout and 'PSCAN' not in result.stdout


async def run_beacon(
    beacon_payload: bytes,
    hci_device: str = 'hci0',
    refresh_interval: int = 30,
):
    ad_data = _build_advertising_data(beacon_payload)
    try:
        while True:
            await asyncio.sleep(refresh_interval)
            if _apply_beacon(ad_data, hci_device):
                print(f"[beacon] Refreshed advertising", flush=True)
    finally:
        stop_beacon(hci_device)
