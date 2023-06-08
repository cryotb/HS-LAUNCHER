#include <include.hpp>

bool hwid::query_disk_property(c_device& device, disk_property_t* out)
{
	VMP_BEGIN_ULTRA("HWID_QueryDiskProperty");

	if (!device.valid())
		return false;

	auto query = STORAGE_PROPERTY_QUERY{};
	auto desc_header = STORAGE_DESCRIPTOR_HEADER{};
	auto request_length = DWORD{};

	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;

	if (!NT_SUCCESS(device.send_control(IOCTL_STORAGE_QUERY_PROPERTY, &query,
		sizeof(query), &desc_header, sizeof(desc_header), &request_length)))
		return false;

	const size_t dd_length = desc_header.Size;
	auto* const dd_buffer = malloc(dd_length);

	if (dd_buffer == nullptr)
		return false;

	memset(dd_buffer, 0, dd_length);

	if (!NT_SUCCESS(device.send_control(IOCTL_STORAGE_QUERY_PROPERTY, &query,
		sizeof(query), dd_buffer, dd_length)))
		return false;

	auto* const device_desc =
		reinterpret_cast<PSTORAGE_DEVICE_DESCRIPTOR>(dd_buffer);

	auto* const version_offset =
		reinterpret_cast<char*>((uintptr_t)device_desc + device_desc->Version);

	auto* const serial_number =
		reinterpret_cast<char*>((uintptr_t)device_desc + device_desc->SerialNumberOffset);

	memcpy(out->serial_number, serial_number, 20);
	memcpy(out->version_offset, version_offset, 20);

	free(dd_buffer);

	VMP_END();

	return true;
}

bool hwid::collect_all(identifiers_t* buffer)
{
	VMP_BEGIN_ULTRA("HWID_CollectAll");

	//
	// Physical Drive Hardware IDs
	//
	const auto collect_disk = [&buffer]()
	{
		auto main_disk = c_device(_XS("PhysicalDrive0"));

		if (!main_disk.valid())
			return false;

		if (!hwid::query_disk_property(main_disk,
			&buffer->disk))
			return false;

		return true;
	};

	//
	// Physical Network Interface Hardware IDs
	//
	const auto collect_nic = [&buffer]
	{
		const auto nic_list = get_nic_list();
		const auto& main_nic = nic_list.front();

		std::string pa{}, ca{};

		if (!get_nic_address(main_nic->AdapterName, pa, ca))
			return false;

		memcpy(buffer->nic.current_address, ca.data(), ca.size());
		memcpy(buffer->nic.permanent_address, pa.data(), ca.size());

		return true;
	};

	if (!collect_disk() || !collect_nic())
		return false;

	VMP_END();

	return true;
}

uint32_t hwid::compute_hardware_id()
{
	VMP_BEGIN_ULTRA("HWID_ComputeHardwareId");

	auto hwid = identifiers_t{};
	memset(&hwid, 0, sizeof(hwid));

	if (!collect_all(&hwid))
		return 0;

	const auto result =
		CRC::Calculate(&hwid, sizeof(hwid), CRC::CRC_32());

	VMP_END();

	return result;
}
