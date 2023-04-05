# Copyright 2016 - 2021 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials, and your use of them is governed by the
# express license under which they were provided to you (License). Unless the License provides otherwise, you may not
# use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's
# prior written permission.
#
# This software and the related documents are provided as is, with no express or implied warranties, other than those
# that are expressly stated in the License.


import subprocess
import os
import csv
import sys

ENCODING = 'utf-8'

LSBLK_HEADER = ['NAME', 'MAJ:MIN', 'RM', 'SIZE', 'RO', 'TYPE', 'MOUNTPOINT']
EXTRA_FIELDS = ['SOCKET', 'SLOT_ID', 'PCI_ID', 'MOUNTED']
OUTPUT_FIELDS = LSBLK_HEADER + EXTRA_FIELDS
TOTAL_NUM_OF_FIELDS = len(OUTPUT_FIELDS)

IS_MOUNTED = 1
NVME_DEVICE_SUFFIX = 'n1'
FS_SEPARATOR = '/'


def create_subprocess(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)


def run_command(cmd):
    subproc = create_subprocess(cmd)
    out, err = subproc.communicate()
    if subproc.returncode != 0:
        raise Exception('{0} returned {1}. Details: {2}'.format(cmd, subproc.returncode, err))
    return out.decode()


def extend_list(entity, length):
    if len(entity) < length:
        entity.extend(['' for _ in range(length - len(entity))])
    return entity


def equalize_list_lengths(list_of_lists, max_length):
    return [extend_list(candidate_list, max_length) for candidate_list in list_of_lists]


def gather_disks_with_mountpoint():
    disks_with_mountpoint = create_subprocess("lsblk --list --noheadings --output NAME,MOUNTPOINT | awk '$2 != NULL' | "
                                              "awk '{print $1}'")
    return [disk_name.decode(ENCODING).strip() for disk_name in disks_with_mountpoint.stdout.readlines()]


def update_mount_point_field(block_devices, mounted_disks):
    index = OUTPUT_FIELDS.index('MOUNTED')
    # add value if mount-point exists either for the disk / partition
    for i in range(0, len(block_devices)):
        name = block_devices[i][0]
        for disk in mounted_disks:
            if name in disk:
                block_devices[i][index] = IS_MOUNTED
                break


def update_list(device, field, value):
    index = OUTPUT_FIELDS.index(field)
    device[index] = value


def gather_block_devices():
    block_devices_info = create_subprocess("lsblk --nodeps --noheadings --list --output " +
                                           ','.join(LSBLK_HEADER) + " | grep 'disk'")
    devices = [device.decode(ENCODING).split() for device in block_devices_info.stdout.readlines()]
    return equalize_list_lengths(devices, TOTAL_NUM_OF_FIELDS)


def identify_nvme_commands():
    # defaults if nvme devices don't exist
    list_cmd = None
    socket_cmd = None

    nvme_block_cmd_ret_code = os.system("ls -d /sys/bus/pci/devices/*/block/nvme* >/dev/null 2>&1")
    nvme_generic_cmd_ret_code = os.system("ls -d /sys/bus/pci/devices/*/nvme* >/dev/null 2>&1")

    if nvme_block_cmd_ret_code == 0:
        list_cmd = "ls -d /sys/bus/pci/devices/*/block/nvme*"
        socket_cmd = "cat /sys/bus/pci/devices/*/block/nvme*/device/numa_node"
    elif nvme_generic_cmd_ret_code == 0:
        list_cmd = "ls -d /sys/bus/pci/devices/*/nvme/nvm*"
        socket_cmd = "cat /sys/bus/pci/devices/*/nvme/nvm*/device/numa_node"

    return list_cmd, socket_cmd


def obtain_extra_fields(block_devices, list_cmd, socket_cmd):
    pci_cmd = "lspci -nn | grep 'Non-Volatile' | cut -d ']' -f2 | cut -d '[' -f2 | cut -d ']' -f1"
    slot_idx = 5
    slot_max_split = 7
    name_idx = 1
    name_max_split = 1

    if list_cmd is not None:
        socket = run_command(socket_cmd).strip().split()[0]
        pci_id_number = run_command(pci_cmd).strip().strip('[]').split()[0]
        subproc = create_subprocess(list_cmd)
        for device in subproc.stdout.readlines():
            device = device.decode(ENCODING)
            slot_id = device.split(FS_SEPARATOR, slot_max_split)[slot_idx].strip()
            dev_name = device.rsplit(FS_SEPARATOR, name_max_split)[name_idx].strip() + NVME_DEVICE_SUFFIX
            for i in range(0, len(block_devices)):
                if (block_devices[i][0]) in dev_name:
                    update_list(block_devices[i], 'SLOT_ID', slot_id)
                    update_list(block_devices[i], 'SOCKET', socket)
                    update_list(block_devices[i], 'PCI_ID', pci_id_number)


def write_to_output_file(list_of_tuples_disks):
    writer = csv.DictWriter(sys.stdout, fieldnames=OUTPUT_FIELDS)
    # the following is a python 2.6 equivalent of writer.writeheader()
    writer.writerow(dict((fieldname, fieldname) for fieldname in OUTPUT_FIELDS))
    for (name, num, rm, size, ro, type, mount_point, socket, slot_id, pci_id, mounted) in list_of_tuples_disks:
        if name.startswith("pmem"):
          continue
        writer.writerow({OUTPUT_FIELDS[0]: name, OUTPUT_FIELDS[1]: num, OUTPUT_FIELDS[2]: rm, OUTPUT_FIELDS[3]: size,
                         OUTPUT_FIELDS[4]: ro, OUTPUT_FIELDS[5]: type, OUTPUT_FIELDS[6]: mount_point,
                         OUTPUT_FIELDS[7]: socket, OUTPUT_FIELDS[8]: slot_id, OUTPUT_FIELDS[9]: pci_id,
                         OUTPUT_FIELDS[10]: mounted})


def main():
    block_devices = gather_block_devices()
    mounted_disks = gather_disks_with_mountpoint()
    list_cmd, socket_cmd = identify_nvme_commands()
    update_mount_point_field(block_devices, mounted_disks)
    obtain_extra_fields(block_devices, list_cmd, socket_cmd)
    list_of_tuples_disks = [tuple(d) for d in block_devices]
    write_to_output_file(list_of_tuples_disks)


if __name__ == '__main__':
    main()
