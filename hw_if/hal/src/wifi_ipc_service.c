#ifdef __MEOS__
    #include "meos/ipc_service/wifi_ipc_service.h"
    #include "MEOS.h"
#else
    #include "wifi_ipc_service.h"
#endif /* __MEOS__ */

#ifdef __MEOS__
static void mvdmaCopyRegionBlocking(uint32_t snkStartAddr, uint32_t snkEndAddr, uint32_t srcStartAddr)
{
    IRQ_IPL_T ipl = IRQ_raiseIPL();

    volatile uint32_t srcJobPtr[4] = { 0 };
    volatile uint32_t snkJobPtr[4] = { 0 };
    uint32_t size = snkEndAddr - snkStartAddr;

    if (size == 0)
    {
        IRQ_restoreIPL(ipl);
        return;
    }

    const uint32_t plainDataAttr = 7;
    uint32_t attrSize = (plainDataAttr << 24) | size;

    srcJobPtr[0] = srcStartAddr;
    srcJobPtr[1] = attrSize;
    snkJobPtr[0] = snkStartAddr;
    snkJobPtr[1] = attrSize;

    NRF_WIFICORE_MVDMA->SOURCE.LISTPTR = (uint32_t)srcJobPtr;
    NRF_WIFICORE_MVDMA->SINK.LISTPTR = (uint32_t)snkJobPtr;
    asm volatile("fence");

    NRF_WIFICORE_MVDMA->TASKS_START[0] = 1;

    while (!NRF_WIFICORE_MVDMA->EVENTS_END)
    {
        asm volatile("nop");
    }

    NRF_WIFICORE_MVDMA->EVENTS_END = 0;
    NRF_WIFICORE_MVDMA->EVENTS_STARTED = 0;

    IRQ_restoreIPL(ipl);
}
#endif /* __MEOS__ */

static void wifi_ipc_ep_bound(void *priv)
{
    WIFI_IPC_T *p_context = (WIFI_IPC_T *) priv;
    p_context->busy_q.ipc_ready = true;
}

static void wifi_ipc_recv_callback(const void *data, size_t len, void *priv)
{
    (void) len;
    uint32_t global_addr = *((uint32_t *) data);

    WIFI_IPC_T *p_context = (WIFI_IPC_T *) priv;
    p_context->busy_q.recv_cb((void *) global_addr, p_context->busy_q.priv);

    if (p_context->free_q != NULL)
    {
        while(!SPSC32_push(p_context->free_q, global_addr))
            ;
    }
}

static void wifi_ipc_busyq_init(WIFI_IPC_BUSYQ_T* p_busyq, const IPC_DEVICE_WRAPPER_T *ipc_inst, void *rx_cb, void *priv)
{
    p_busyq->ipc_inst = ipc_inst;
    p_busyq->ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound;
    p_busyq->ipc_ep_cfg.cb.received = wifi_ipc_recv_callback;
    p_busyq->recv_cb = rx_cb;
    p_busyq->ipc_ready = false;
    p_busyq->priv = priv;
}

/**
 * Register the IPC service on the busy_queue
 */
static WIFI_IPC_STATUS_T wifi_ipc_busyq_register(WIFI_IPC_T *p_context)
{
    int ret;
    struct device *ipc_instance = GET_IPC_INSTANCE(p_context->busy_q.ipc_inst);

    ret = ipc_service_open_instance(ipc_instance);
    if (ret < 0)
    {
        return WIFI_IPC_STATUS_INIT_ERR;
    }

    p_context->busy_q.ipc_ep_cfg.name = "ep";
    p_context->busy_q.ipc_ep_cfg.priv = p_context;

    ret = ipc_service_register_endpoint(ipc_instance, &p_context->busy_q.ipc_ep, &p_context->busy_q.ipc_ep_cfg);
    if (ret < 0 && ret != -EALREADY)
    {
        return WIFI_IPC_STATUS_INIT_ERR;
    }

    printf("ok\n");
    return WIFI_IPC_STATUS_OK;
}

/*******************************************************************************
 *
 * Common functions
 *
 ******************************************************************************/

#ifdef __MEOS__
void wifi_ipc_mvdma_copy(void *p_dest, const void *p_src, size_t len)
{
    uint32_t dest_addr = (uint32_t) p_dest;
    uint32_t src_addr = (uint32_t) p_src;
    mvdmaCopyRegionBlocking(dest_addr, dest_addr + len, src_addr);
}
#endif /* __MEOS__ */

WIFI_IPC_STATUS_T wifi_ipc_bind_ipc_service(WIFI_IPC_T *p_context,
    const IPC_DEVICE_WRAPPER_T *ipc_inst, void (*rx_cb)(void *data, void *priv), void *priv)
{
    wifi_ipc_busyq_init(&p_context->busy_q, ipc_inst, rx_cb, priv);
    return wifi_ipc_busyq_register(p_context);
}

WIFI_IPC_STATUS_T wifi_ipc_bind_ipc_service_tx_rx(WIFI_IPC_T *p_tx, WIFI_IPC_T *p_rx,
    const IPC_DEVICE_WRAPPER_T *ipc_inst, void (*rx_cb)(void *data, void *priv), void *priv)
{
    wifi_ipc_busyq_init(&p_rx->busy_q, ipc_inst, rx_cb, priv);

    /**
     * When initialising an IPC service, both TX and RX mailboxes need to be
     * registered at the same time using a single function call. Both p_tx and
     * p_rx need to refer to the same IPC instance.
     */
    p_tx->linked_ipc = &p_rx->busy_q;

    return wifi_ipc_busyq_register(p_rx);
}

WIFI_IPC_STATUS_T wifi_ipc_freeq_get(WIFI_IPC_T *p_context, uint32_t *data)
{
    if (p_context->free_q == NULL)
    {
        return WIFI_IPC_STATUS_FREEQ_UNINIT_ERR;
    }

    if (SPSC32_isEmpty(p_context->free_q))
    {
        return WIFI_IPC_STATUS_FREEQ_EMPTY;
    }

    if (!SPSC32_readHead(p_context->free_q, data))
    {
        return WIFI_IPC_STATUS_FREEQ_EMPTY;
    }

    return WIFI_IPC_STATUS_OK;
}

WIFI_IPC_STATUS_T wifi_ipc_freeq_send(WIFI_IPC_T *p_context, uint32_t data)
{
    return (
        SPSC32_push(p_context->free_q, data) == true
            ? WIFI_IPC_STATUS_OK
            : WIFI_IPC_STATUS_FREEQ_FULL
    );
}

WIFI_IPC_STATUS_T wifi_ipc_busyq_send(WIFI_IPC_T *p_context, uint32_t *data)
{
    /* Get correct linked endpoint */
    WIFI_IPC_BUSYQ_T *p_busyq = p_context->linked_ipc ?
                                p_context->linked_ipc : &p_context->busy_q;

    if (!p_busyq->ipc_ready)
    {
        return WIFI_IPC_STATUS_BUSYQ_NOTREADY;
    }

    int ret = ipc_service_send(&p_busyq->ipc_ep, data, sizeof(*data));

    if (ret == -ENOMEM)
    {
        /* No space in the buffer */
        return WIFI_IPC_STATUS_BUSYQ_FULL;
    }
    else if (ret < 0)
    {
        /* Critical IPC failure */
        return WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR;
    }

    if (p_context->free_q != NULL)
    {
        /* Free up global address pointer from the free queue */
        uint32_t dataOut;
        return (
            SPSC32_pop(p_context->free_q, &dataOut) == true
                ? (*data == dataOut ? WIFI_IPC_STATUS_OK : WIFI_IPC_STATUS_FREEQ_INVALID)
                : WIFI_IPC_STATUS_FREEQ_EMPTY
        );
    }

    return WIFI_IPC_STATUS_OK;
}

/*******************************************************************************
 *
 * HOST specific functions
 *
 ******************************************************************************/

WIFI_IPC_STATUS_T wifi_ipc_host_cmd_init(WIFI_IPC_T *p_context, uint32_t addr_freeq)
{
    p_context->free_q = (void *) addr_freeq;
    return WIFI_IPC_STATUS_OK;
}

WIFI_IPC_STATUS_T wifi_ipc_host_event_init(WIFI_IPC_T *p_context, uint32_t addr_freeq)
{
    p_context->free_q = (void *) addr_freeq;
    return WIFI_IPC_STATUS_OK;
}

WIFI_IPC_STATUS_T wifi_ipc_host_cmd_get(WIFI_IPC_T *p_context, uint32_t *p_data)
{
    return wifi_ipc_freeq_get(p_context, p_data);
}

WIFI_IPC_STATUS_T wifi_ipc_host_cmd_send(WIFI_IPC_T *p_context, uint32_t *p_data)
{
    return wifi_ipc_busyq_send(p_context, p_data);
}

WIFI_IPC_STATUS_T wifi_ipc_host_cmd_send_memcpy(WIFI_IPC_T *p_context, void *p_msg, size_t len)
{
    int ret;
    uint32_t gdram_addr;

    ret = wifi_ipc_host_cmd_get(p_context, &gdram_addr);
    if (ret != WIFI_IPC_STATUS_OK)
    {
        return ret;
    }

    memcpy((void *) gdram_addr, p_msg, len);

    return wifi_ipc_host_cmd_send(p_context, &gdram_addr);
}

WIFI_IPC_STATUS_T wifi_ipc_host_tx_send(WIFI_IPC_T *p_context, void *p_msg)
{
    return wifi_ipc_host_cmd_send(p_context, (uint32_t *) &p_msg);
}

/*******************************************************************************
 *
 * RPU specific functions
 *
 ******************************************************************************/

WIFI_IPC_STATUS_T wifi_ipc_rpu_event_init(WIFI_IPC_T *p_context, uint32_t addr_freeq, uint32_t addr_gdram)
{
    p_context->free_q = SPSC32_init(addr_freeq, WIFI_IPC_EVENT_SPSC_SIZE);

    for (size_t i = 0; i < WIFI_IPC_EVENT_NUM; i++)
    {
        int ret = wifi_ipc_freeq_send(p_context, addr_gdram);
        if (ret != WIFI_IPC_STATUS_OK)
        {
            return ret;
        }

        addr_gdram += WIFI_IPC_EVENT_SIZE;
    }

    return WIFI_IPC_STATUS_OK;
}

WIFI_IPC_STATUS_T wifi_ipc_rpu_cmd_init(WIFI_IPC_T *p_context, uint32_t addr_freeq, uint32_t addr_gdram)
{
    p_context->free_q = SPSC32_init(addr_freeq, WIFI_IPC_CMD_SPSC_SIZE);

    for (size_t i = 0; i < WIFI_IPC_CMD_NUM; i++)
    {
        int ret = wifi_ipc_freeq_send(p_context, addr_gdram);
        if (ret != WIFI_IPC_STATUS_OK)
        {
            return ret;
        }

        addr_gdram += WIFI_IPC_CMD_SIZE;
    }

    return WIFI_IPC_STATUS_OK;
}

WIFI_IPC_STATUS_T wifi_ipc_rpu_event_get(WIFI_IPC_T *p_context, uint32_t *p_data)
{
    return wifi_ipc_freeq_get(p_context, p_data);
}

WIFI_IPC_STATUS_T wifi_ipc_rpu_event_send(WIFI_IPC_T *p_context, uint32_t *p_data)
{
    return wifi_ipc_busyq_send(p_context, p_data);
}

WIFI_IPC_STATUS_T wifi_ipc_rpu_event_send_mvdma(WIFI_IPC_T *p_context, void *p_msg, size_t len)
{
    int ret;
    uint32_t gdram_addr;

    ret = wifi_ipc_rpu_event_get(p_context, &gdram_addr);
    if (ret != WIFI_IPC_STATUS_OK)
    {
        return ret;
    }

    /* MVDMA copy local msg to global ram */
    wifi_ipc_mvdma_copy((void *) gdram_addr, p_msg, len);

    return wifi_ipc_rpu_event_send(p_context, &gdram_addr);
}

WIFI_IPC_STATUS_T wifi_ipc_rpu_tx_send(WIFI_IPC_T *p_context, void *p_msg)
{
    return wifi_ipc_rpu_event_send(p_context, (uint32_t *) &p_msg);
}