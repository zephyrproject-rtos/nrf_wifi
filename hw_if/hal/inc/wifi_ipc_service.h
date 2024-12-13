#ifndef WIFI_IPC_SERVICE_H
#define WIFI_IPC_SERVICE_H

#ifdef __MEOS__
    #include "meos/ipc_service/ipc_service.h"
    #include "meos/ipc_service/spsc_qm.h"

    typedef struct ipc_device_wrapper IPC_DEVICE_WRAPPER_T;
#else
    #include <zephyr/device.h>
    #include <zephyr/ipc/ipc_service.h>
    #include "spsc_qm.h"

    #define GET_IPC_INSTANCE(dev) (dev)
    typedef struct device IPC_DEVICE_WRAPPER_T;
#endif /* __MEOS__ */

#include <stdint.h>
#include <stdbool.h>

/*
 * Must be large enough to contain the internal struct (spsc_pbuf struct) and
 * at least two bytes of data (one is reserved for written message length)
 */
#define _MIN_SPSC_SIZE              (sizeof(SPSC_QUEUE_T) + sizeof(uint32_t))
/*
 * TODO: Unsure why some additional bytes are needed for overhead.
 */
#define WIFI_IPC_GET_SPSC_SIZE(x)   (_MIN_SPSC_SIZE + 12 + (x))

/* 4 x cmd location 32-bit pointers of 400 bytes each */
#define WIFI_IPC_CMD_SIZE           400
#define WIFI_IPC_CMD_NUM            4
#define WIFI_IPC_CMD_SPSC_SIZE      WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_CMD_NUM * sizeof(uint32_t))

/* 7 x event location 32-bit pointers of 1000 bytes each */
#define WIFI_IPC_EVENT_SIZE         1000
#define WIFI_IPC_EVENT_NUM          7
#define WIFI_IPC_EVENT_SPSC_SIZE    WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_EVENT_NUM * sizeof(uint32_t))

/**
 * Status code.
 */
typedef enum
{
    WIFI_IPC_STATUS_OK = 0,
    WIFI_IPC_STATUS_INIT_ERR,           /* (Error) Fail to register IPC service for Busy queue. */
    WIFI_IPC_STATUS_FREEQ_UNINIT_ERR,   /* (Error) Free queue has not been initialised */
    WIFI_IPC_STATUS_FREEQ_EMPTY,        /* Free queue is empty. */
    WIFI_IPC_STATUS_FREEQ_INVALID,      /* Value passed to wifi_ipc_busyq_send() does not match with value from the Free queue. */
    WIFI_IPC_STATUS_FREEQ_FULL,         /* Free queue is full. */
    WIFI_IPC_STATUS_BUSYQ_NOTREADY,     /* IPC service for Busy queue connection has not been established. */
    WIFI_IPC_STATUS_BUSYQ_FULL,         /* Busy queue is full. */
    WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR, /* (Error) IPC transfer failure. Should never happen. */
} WIFI_IPC_STATUS_T;

/**
 * Structure to hold context information for busy queue.
 */
typedef struct
{
    const IPC_DEVICE_WRAPPER_T *ipc_inst;
    struct ipc_ept ipc_ep;
    struct ipc_ept_cfg ipc_ep_cfg;
    void (*recv_cb)(const void *data, const void *priv);
    const void *priv;
    volatile bool ipc_ready;
} WIFI_IPC_BUSYQ_T;

/**
 * Top-level structure to hold context information for sending data between RPU
 * and the Host.
 */
typedef struct
{
    SPSC_QUEUE_T *free_q;

    WIFI_IPC_BUSYQ_T busy_q;
    WIFI_IPC_BUSYQ_T *linked_ipc;
} WIFI_IPC_T;

/*******************************************************************************
 *
 * Common functions
 *
 ******************************************************************************/

/**
 * Performs memory-to-memory copy via MVDMA.
 *
 * Enters low power state by issuing wait-for-interrupt (WFI) while waiting for
 * MVDMA event to complete.
 *
 * @param[in] p_dest : Pointer to destination memory to be copied to.
 * @param[in] p_src : Pointer to source memory to be copied from.
 * @param[in] len : Number of bytes to be copied.
 */
void wifi_ipc_mvdma_copy(void *p_dest, const void *p_src, size_t len);

/**
 * Bind either TX or RX context to one IPC service. This utilises the half-duplex
 * capability of the IPC service.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] ipc_inst : Pointer to the IPC instance.
 * @param[in] rx_cb : If binding RX context, this is the callback function.
 *                    Leave NULL if binding to a TX.
 * @param[in] priv : If binding RX context, this is the private data to be passed
 *                   along with the callback function.
 *                   Leave NULL if binding to a TX.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_INIT_ERR.
 */
WIFI_IPC_STATUS_T wifi_ipc_bind_ipc_service(WIFI_IPC_T *p_context,
    const IPC_DEVICE_WRAPPER_T *ipc_inst, void (*rx_cb)(void *data, void *priv), void *priv);

/**
 * Bind both TX and RX contexts to a single IPC service. This utilises the
 * full-duplex capability of the IPC service.
 *
 * @param[in] p_tx : Pointer to WIFI_IPC_T struct to bind IPC TX mailbox to.
 * @param[in] p_rx : Pointer to WIFI_IPC_T struct to bind IPC RX mailbox to.
 * @param[in] ipc_inst : Pointer to the IPC instance.
 * @param[in] rx_cb : Callback function to bind to data in received from RX mailbox.
 * @param[in] priv : Private data to the callback function.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_INIT_ERR.
 */
WIFI_IPC_STATUS_T wifi_ipc_bind_ipc_service_tx_rx(WIFI_IPC_T *p_tx, WIFI_IPC_T *p_rx,
    const IPC_DEVICE_WRAPPER_T *ipc_inst, void (*rx_cb)(void *data, void *priv), void *priv);

/**
 * Get data from the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[out] data : Pointer to the data to read to.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_FREEQ_EMPTY.
 */
WIFI_IPC_STATUS_T wifi_ipc_freeq_get(WIFI_IPC_T *p_context, uint32_t *data);

/**
 * Send data to the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] data : 32-bit data to send.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_FREEQ_FULL.
 */
WIFI_IPC_STATUS_T wifi_ipc_freeq_send(WIFI_IPC_T *p_context, uint32_t data);

/**
 * Send data to the busy queue over IPC service, and pop the same data from the
 * free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] data : Pointer to the data to send to.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise one of the followings:
 *              - WIFI_IPC_STATUS_BUSYQ_NOTREADY
 *              - WIFI_IPC_STATUS_BUSYQ_FULL
 *              - WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR
 *              - WIFI_IPC_STATUS_FREEQ_INVALID
 *              - WIFI_IPC_STATUS_FREEQ_EMPTY
 */
WIFI_IPC_STATUS_T wifi_ipc_busyq_send(WIFI_IPC_T *p_context, uint32_t *data);

/*******************************************************************************
 *
 * HOST specific functions
 *
 ******************************************************************************/

/**
 * Prepares and initialises the Host for sending a command to RPU.
 *
 * The free queue points to the already allocated free queue from the RPU.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] addr_freeq : Address of the allocated free queue.
 * @return : WIFI_IPC_STATUS_OK if successful.
 */
WIFI_IPC_STATUS_T wifi_ipc_host_cmd_init(WIFI_IPC_T *p_context, uint32_t addr_freeq);

/**
 * Prepares and initialises the Host for receiving an event from RPU.
 *
 * The free queue points to the already allocated free queue from the RPU.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] addr_freeq : Address of the allocated SPSC free queue.
 * @return : WIFI_IPC_STATUS_OK if successful.
 */
WIFI_IPC_STATUS_T wifi_ipc_host_event_init(WIFI_IPC_T *p_context, uint32_t addr_freeq);

/**
 * Get a command location from the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[out] p_data : Pointer to data to write event location to.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_FREEQ_EMPTY.
 */
WIFI_IPC_STATUS_T wifi_ipc_host_cmd_get(WIFI_IPC_T *p_context, uint32_t *p_data);

/**
 * Send an event location pointer to the Host and frees up the event location
 * pointer from the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_data : Pointer to command location to be sent.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise one of the followings
 *              - WIFI_IPC_STATUS_BUSYQ_NOTREADY
 *              - WIFI_IPC_STATUS_BUSYQ_FULL
 *              - WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR
 *              - WIFI_IPC_STATUS_FREEQ_INVALID
 *              - WIFI_IPC_STATUS_FREEQ_EMPTY
 */
WIFI_IPC_STATUS_T wifi_ipc_host_cmd_send(WIFI_IPC_T *p_context, uint32_t *p_data);

/**
 * Send a command from the Host to RPU using standard memcpy.
 *
 * 1. Retrieves an address pointer of Packet RAM from the free queue.
 * 2. Copies local message to the retrieved address pointer via memcpy.
 * 3. Sends the address pointer to the busy queue via IPC service.
 * 4. Upon successful transmission, removes the address pointer the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_msg : Pointer the local message to be copied to Packet RAM via memcpy.
 * @param[in] len : Length of the local message in bytes.
 * @return : WIFI_IPC_STATUS_OK if send is successful, otherwise one of
 *           status code from WIFI_IPC_STATUS_T will be returned.
 */
WIFI_IPC_STATUS_T wifi_ipc_host_cmd_send_memcpy(WIFI_IPC_T *p_context, void *p_msg, size_t len);

/**
 * Send a tx data pointer from the Host to RPU and raises RPU interrupt.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_msg : Pointer to message.
 * @return : WIFI_IPC_STATUS_OK if send is successful, otherwise one of
 *           status code from WIFI_IPC_STATUS_T will be returned.
 */
WIFI_IPC_STATUS_T wifi_ipc_host_tx_send(WIFI_IPC_T *p_context, void *p_msg);

/*******************************************************************************
 *
 * RPU specific functions
 *
 ******************************************************************************/

/**
 * Prepares and initialises RPU for sending an event to the Host.
 *
 * The free queue will be created and allocated capable of holding 7 event
 * location pointers from the Packet RAM. This free queue then gets populated
 * starting from address @p addr_gdram.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] addr_freeq : Address of the SPSC free queue to allocate.
 * @param[in] addr_gdram : Start address of the packet RAM to populate the SPSC free queue.
 * @return : WIFI_IPC_STATUS_OK if successful.
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_event_init(WIFI_IPC_T *p_context, uint32_t addr_freeq, uint32_t addr_gdram);

/**
 * Prepares and initialises RPU for receiving a command from the Host.
 *
 * The free queue will be created and allocated capable of holding 4 command
 * location pointers from the Packet RAM. This free queue then gets populated
 * starting from address @p addr_gdram.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] addr_freeq : Address of the SPSC free queue to allocate.
 * @param[in] addr_gdram : Start address of the packet RAM to populate the SPSC free queue.
 * @return : WIFI_IPC_STATUS_OK if successful.
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_cmd_init(WIFI_IPC_T *p_context, uint32_t addr_freeq, uint32_t addr_gdram);

/**
 * Get an event location from the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[out] p_data : Pointer to data to write event location to.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise WIFI_IPC_STATUS_FREEQ_EMPTY.
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_event_get(WIFI_IPC_T *p_context, uint32_t *p_data);

/**
 * Send an event location pointer to the Host and frees up the event location
 * pointer from the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_data : Pointer to event location to be sent.
 * @return : WIFI_IPC_STATUS_OK if successful, otherwise one of the followings
 *              - WIFI_IPC_STATUS_BUSYQ_NOTREADY
 *              - WIFI_IPC_STATUS_BUSYQ_FULL
 *              - WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR
 *              - WIFI_IPC_STATUS_FREEQ_INVALID
 *              - WIFI_IPC_STATUS_FREEQ_EMPTY
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_event_send(WIFI_IPC_T *p_context, uint32_t *p_data);

/**
 * Send an event from RPU to the Host using MVDMA.
 *
 * 1. Retrieves an address pointer of Packet RAM from the free queue.
 * 2. Copies local message to the retrieved address pointer via MVDMA.
 * 3. Sends the address pointer to the busy queue via IPC service.
 * 4. Upon successful transmission, removes the address pointer the free queue.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_msg : Pointer the local message to be copied to Packet RAM via MVDMA.
 * @param[in] len : Length of the local message in bytes.
 * @return : WIFI_IPC_STATUS_OK if send is successful, otherwise one of
 *           status code from WIFI_IPC_STATUS_T will be returned.
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_event_send_mvdma(WIFI_IPC_T *p_context, void *p_msg, size_t len);

/**
 * Send a tx data pointer from RPU to the Host and raises the Host interrupt.
 *
 * @param[in] p_context : Pointer to WIFI_IPC_T struct.
 * @param[in] p_msg : Pointer to message.
 * @return : WIFI_IPC_STATUS_OK if send is successful, otherwise one of
 *           status code from WIFI_IPC_STATUS_T will be returned.
 */
WIFI_IPC_STATUS_T wifi_ipc_rpu_tx_send(WIFI_IPC_T *p_context, void *p_msg);

#endif /* WIFI_IPC_SERVICE_H */
