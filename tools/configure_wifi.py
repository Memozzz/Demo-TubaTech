"""Create local MASTER Wi-Fi configuration; never prints the credential."""
from pathlib import Path
import argparse, getpass, json, secrets

def render_config(password):
    if not 8 <= len(password) <= 63 or not password.isascii() or any(ord(c) < 32 for c in password):
        raise ValueError('Use 8..63 printable ASCII characters')
    return '#pragma once\n// LOCAL SECRET: ignored by Git. Do not publish.\n#define TUBATECH_AP_PASSWORD ' + json.dumps(password) + '\n'

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate',action='store_true',help='Generate a local random credential without displaying it')
    args=parser.parse_args()
    target=Path(__file__).resolve().parents[1]/'firmware/01_MASTER/TubaTech_Integrated_Master/wifi_config.h'
    if target.exists(): raise SystemExit('Local configuration already exists; edit or remove it explicitly to change it')
    password=secrets.token_urlsafe(24) if args.generate else getpass.getpass('Device Wi-Fi password (8..63 characters): ')
    target.write_text(render_config(password),encoding='utf-8')
    print('Created ignored local MASTER Wi-Fi configuration. Keep it private; use its value when joining the AP.')
