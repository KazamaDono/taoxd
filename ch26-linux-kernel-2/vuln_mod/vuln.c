/* excerpt — full source in ch26-linux-kernel-2/vuln_mod/vuln.c */
struct nfx_obj {
    u32               id;
    u32               len;                       /* user-controlled */
    struct list_head  link;
    refcount_t        ref;
    void            (*cb)(struct nfx_obj *);     /* the callable field */
    u8                data[];                    /* flex; kmalloc-1k slab */
};

static struct nfx_obj *nfx_lookup(u32 id)        /* no lock */
{
    struct nfx_obj *o;
    list_for_each_entry(o, &nfx_list, link)
        if (o->id == id) return o;
    return NULL;
}

static long nfx_ioctl(struct file *f, unsigned cmd, unsigned long arg)
{
    struct nfx_req r;
    if (copy_from_user(&r, (void __user *)arg, sizeof r)) return -EFAULT;

    switch (cmd) {
    case NFX_DEL: {                              /* frees under mutex */
        mutex_lock(&nfx_mtx);
        struct nfx_obj *o = nfx_lookup(r.id);
        if (o) { list_del(&o->link); kfree(o); } /* no refcount check */
        mutex_unlock(&nfx_mtx);
        return 0;
    }
    case NFX_FIRE: {                             /* no lock, uses ptr */
        struct nfx_obj *o = nfx_lookup(r.id);
        if (!o) return -ENOENT;
        o->cb(o);                                /* UAF: cb may be freed */
        return 0;
    }
    /* NFX_NEW, NFX_WRITE elided */
    }
    return -EINVAL;
}
