// excerpt: ch27-win-kernel/driver/vuln.c
typedef struct _CH27_BUF {
    SIZE_T Length;         // size of Data[] as agreed at allocate time
    UCHAR  Data[1];        // variable length; total alloc = 16 + Length
} CH27_BUF, *PCH27_BUF;

// IOCTL 0x800: allocate. IOCTL 0x801: write. IOCTL 0x802: free.
NTSTATUS Ch27WriteIoctl(PIRP Irp, PIO_STACK_LOCATION IoSp) {
    ULONG inLen = IoSp->Parameters.DeviceIoControl.InputBufferLength;
    PCH27_WRITE_REQ req = Irp->AssociatedIrp.SystemBuffer;   // ❶

    if (inLen < sizeof(CH27_WRITE_REQ)) return STATUS_BUFFER_TOO_SMALL;

    PCH27_BUF buf = Ch27LookupBuf(req->Handle);              // ❷
    if (!buf) return STATUS_INVALID_HANDLE;

    // BUG: check uses inLen, not buf->Length. An attacker sets
    // inLen big enough to slip past the check, then writes req->Count
    // bytes into buf->Data[] without bounding req->Count by buf->Length.
    if (req->Count > inLen)                                  // ❸ wrong guard
        return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(buf->Data, req->Payload, req->Count);      // ❹ overflow
    return STATUS_SUCCESS;
}
