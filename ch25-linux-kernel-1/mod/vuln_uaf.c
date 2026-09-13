struct blob {
    void (*notify)(struct blob *);   /* ❶ function pointer, fires on read */
    u64   cookie;
    char  data[176];                 /* total sizeof(struct blob) == 0xc0 */
};

/* IOCTL_ALLOC: kmalloc a blob, return a handle */
static long v_alloc(struct v_ctx *ctx, unsigned long arg) {
    int h = idr_alloc(&ctx->handles, NULL, 0, MAX_H, GFP_KERNEL);
    struct blob *b = kmalloc(sizeof(*b), GFP_KERNEL);   /* ❷ kmalloc-192 */
    if (!b) return -ENOMEM;
    b->notify = default_notify;
    idr_replace(&ctx->handles, b, h);
    return h;
}

/* IOCTL_FREE: intended to free one reference; actually frees the object
 * while leaving the handle populated (the bug). */
static long v_free(struct v_ctx *ctx, unsigned long arg) {
    struct blob *b = idr_find(&ctx->handles, arg);
    if (!b) return -ENOENT;
    kfree(b);                         /* ❸ freed here ... */
    /* BUG: idr_remove(&ctx->handles, arg) missing — handle still resolves */
    return 0;
}

/* IOCTL_READ: dereferences whatever the handle now points at */
static long v_read(struct v_ctx *ctx, unsigned long arg) {
    struct blob *b = idr_find(&ctx->handles, arg);
    if (!b) return -ENOENT;
    b->notify(b);                    /* ❹ ... called here, over freed memory */
    return b->cookie;
}
