/* --- v1.0.0 (vulnerable) --- */                          /* ❶ */
int parse_tlv_record(const uint8_t *buf, size_t buflen,
                     tlv_record_t *out)
{
    if (buflen < TLV_HDR_SIZE)                             /* header fits */
        return -EINVAL;
    uint16_t tag = read_be16(buf + 0);
    uint16_t len = read_be16(buf + 2);
    /* MISSING: no check that len <= buflen - TLV_HDR_SIZE */
    memcpy(out->payload, buf + TLV_HDR_SIZE, len);         /* ❸ OOB read */
    out->tag = tag;
    out->len = len;
    return 0;
}

/* --- v1.0.1 (patched) --- */                             /* ❷ */
int parse_tlv_record(const uint8_t *buf, size_t buflen,
                     tlv_record_t *out)
{
    if (buflen < TLV_HDR_SIZE)
        return -EINVAL;
    uint16_t tag = read_be16(buf + 0);
    uint16_t len = read_be16(buf + 2);
    if (len > buflen - TLV_HDR_SIZE)                       /* ❹ the fix */
        return -EMSGSIZE;
    memcpy(out->payload, buf + TLV_HDR_SIZE, len);
    out->tag = tag;
    out->len = len;
    return 0;
}
