#pragma once

#pragma once

namespace hwid
{
    struct disk_property_t
    {
        char version_offset[128];
        char serial_number[128];
    };

    struct nic_property_t
    {
        char current_address[52];
        char permanent_address[52];
    };

    struct identifiers_t
    {
        disk_property_t disk;
        nic_property_t nic;
    };

    inline auto get_nic_list()
    {
        auto result = std::vector<PIP_ADAPTER_INFO>();

        auto length = ULONG{};

        if (GetAdaptersInfo(nullptr, &length) !=
            ERROR_BUFFER_OVERFLOW)
            return result;

        auto* const buffer =
            malloc(length);

        if (buffer == nullptr)
            return result;

        memset(buffer, 0, length);

        if (GetAdaptersInfo(
            (PIP_ADAPTER_INFO)buffer, &length)
            != ERROR_SUCCESS)
            return result;


        auto* entry =
            reinterpret_cast<PIP_ADAPTER_INFO>(buffer);

        do
        {
            result.emplace_back(entry);

            entry = entry->Next;
        } while (entry);

        return result;
    }

    inline auto get_nic_address(char* strServiceName, std::string& perm, std::string& curr)
    {
        VMP_BEGIN_ULTRA("HWID_GetNicAddress");

        BOOL bRet = FALSE;
        char pstrBuf[512];
        wsprintfA(pstrBuf, _XS("\\\\.\\%s"), strServiceName);

        HANDLE hDev = CreateFileA(pstrBuf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
        if (hDev != INVALID_HANDLE_VALUE)
        {
            int inBuf;
            BYTE outBuf[256] = { 0 };
            DWORD BytesReturned;

            inBuf = OID_802_3_PERMANENT_ADDRESS;
            if (DeviceIoControl(hDev, IOCTL_NDIS_QUERY_GLOBAL_STATS, (LPVOID)&inBuf, 4, outBuf, 256, &BytesReturned, NULL))
            {
                sprintf_s(pstrBuf, _XS("%02X-% 02X-% 02X-% 02X-% 02X-% 02X"),
                    outBuf[0], outBuf[1], outBuf[2], outBuf[3], outBuf[4], outBuf[5]);

                perm = pstrBuf;
                bRet = TRUE;
            }

            inBuf = OID_802_3_CURRENT_ADDRESS;
            if (DeviceIoControl(hDev, IOCTL_NDIS_QUERY_GLOBAL_STATS, (LPVOID)&inBuf, 4, outBuf, 256, &BytesReturned, NULL))
            {
                sprintf_s(pstrBuf, _XS("%02X-% 02X-% 02X-% 02X-% 02X-% 02X"),
                    outBuf[0], outBuf[1], outBuf[2], outBuf[3], outBuf[4], outBuf[5]);

                curr = pstrBuf;
                bRet = TRUE;
            }

            CloseHandle(hDev);
        }

        VMP_END();

        return bRet;
    }

    extern bool query_disk_property(c_device& device, disk_property_t* out);
    extern bool collect_all(identifiers_t* buffer);
    extern uint32_t compute_hardware_id();
}

namespace k
{

    //
    // IDENTIFY data (from ATAPI driver source)
    //

#pragma pack(1)

    typedef struct _IDENTIFY_DATA {
        USHORT GeneralConfiguration;            // 00 00
        USHORT NumberOfCylinders;               // 02  1
        USHORT Reserved1;                       // 04  2
        USHORT NumberOfHeads;                   // 06  3
        USHORT UnformattedBytesPerTrack;        // 08  4
        USHORT UnformattedBytesPerSector;       // 0A  5
        USHORT SectorsPerTrack;                 // 0C  6
        USHORT VendorUnique1[3];                // 0E  7-9
        USHORT SerialNumber[10];                // 14  10-19
        USHORT BufferType;                      // 28  20
        USHORT BufferSectorSize;                // 2A  21
        USHORT NumberOfEccBytes;                // 2C  22
        USHORT FirmwareRevision[4];             // 2E  23-26
        USHORT ModelNumber[20];                 // 36  27-46
        UCHAR  MaximumBlockTransfer;            // 5E  47
        UCHAR  VendorUnique2;                   // 5F
        USHORT DoubleWordIo;                    // 60  48
        USHORT Capabilities;                    // 62  49
        USHORT Reserved2;                       // 64  50
        UCHAR  VendorUnique3;                   // 66  51
        UCHAR  PioCycleTimingMode;              // 67
        UCHAR  VendorUnique4;                   // 68  52
        UCHAR  DmaCycleTimingMode;              // 69
        USHORT TranslationFieldsValid : 1;        // 6A  53
        USHORT Reserved3 : 15;
        USHORT NumberOfCurrentCylinders;        // 6C  54
        USHORT NumberOfCurrentHeads;            // 6E  55
        USHORT CurrentSectorsPerTrack;          // 70  56
        ULONG  CurrentSectorCapacity;           // 72  57-58
        USHORT CurrentMultiSectorSetting;       //     59
        ULONG  UserAddressableSectors;          //     60-61
        USHORT SingleWordDMASupport : 8;        //     62
        USHORT SingleWordDMAActive : 8;
        USHORT MultiWordDMASupport : 8;         //     63
        USHORT MultiWordDMAActive : 8;
        USHORT AdvancedPIOModes : 8;            //     64
        USHORT Reserved4 : 8;
        USHORT MinimumMWXferCycleTime;          //     65
        USHORT RecommendedMWXferCycleTime;      //     66
        USHORT MinimumPIOCycleTime;             //     67
        USHORT MinimumPIOCycleTimeIORDY;        //     68
        USHORT Reserved5[2];                    //     69-70
        USHORT ReleaseTimeOverlapped;           //     71
        USHORT ReleaseTimeServiceCommand;       //     72
        USHORT MajorRevision;                   //     73
        USHORT MinorRevision;                   //     74
        USHORT Reserved6[50];                   //     75-126
        USHORT SpecialFunctionsEnabled;         //     127
        USHORT Reserved7[128];                  //     128-255
    } IDENTIFY_DATA, * PIDENTIFY_DATA;

#pragma pack()
}

