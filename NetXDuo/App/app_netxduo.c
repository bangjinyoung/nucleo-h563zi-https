/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
#include "nxd_dhcp_client.h"
/* USER CODE BEGIN Includes */
#include "printf.h"

#include "nxd_dns.h"
#include "nx_web_http_client.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      NxAppThread;
NX_PACKET_POOL NxAppPool;
NX_IP          NetXDuoEthIpInstance;
TX_SEMAPHORE   DHCPSemaphore;
NX_DHCP        DHCPClient;
/* USER CODE BEGIN PV */
#define DNS_SERVER_ADDRESS IP_ADDRESS(168,126,63,1) // KT DNS

NX_DNS client_dns;

UCHAR record_buffer[256];
UINT record_count, i;
ULONG* ipv4_address_ptr[10];

UCHAR local_cache[1024];

NX_WEB_HTTP_CLIENT http_client;
unsigned char buffer[1024 * 20];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_NetXDuo_MEM_POOL */
  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  CHAR *pointer;
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */

  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */

  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Create the DHCP client */

  /* USER CODE BEGIN DHCP_Protocol_Initialization */

  /* USER CODE END DHCP_Protocol_Initialization */

  ret = nx_dhcp_create(&DHCPClient, &NetXDuoEthIpInstance, "DHCP Client");

  if (ret != NX_SUCCESS)
  {
    return NX_DHCP_ERROR;
  }

  /* set DHCP notification callback  */
  tx_semaphore_create(&DHCPSemaphore, "DHCP Semaphore", 0);

  /* USER CODE BEGIN MX_NetXDuo_Init */
  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/**
* @brief  ip address change callback.
* @param ip_instance: NX_IP instance
* @param ptr: user data
* @retval none
*/
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
  /* USER CODE BEGIN ip_address_change_notify_callback */

  /* USER CODE END ip_address_change_notify_callback */

  /* release the semaphore as soon as an IP address is available */
  tx_semaphore_put(&DHCPSemaphore);
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID nx_app_thread_entry (ULONG thread_input)
{
  /* USER CODE BEGIN Nx_App_Thread_Entry 0 */

  /* USER CODE END Nx_App_Thread_Entry 0 */

  UINT ret = NX_SUCCESS;

  /* USER CODE BEGIN Nx_App_Thread_Entry 1 */

  /* USER CODE END Nx_App_Thread_Entry 1 */

  /* register the IP address change callback */
  ret = nx_ip_address_change_notify(&NetXDuoEthIpInstance, ip_address_change_notify_callback, NULL);
  if (ret != NX_SUCCESS)
  {
    /* USER CODE BEGIN IP address change callback error */

    /* USER CODE END IP address change callback error */
  }

  /* start the DHCP client */
  ret = nx_dhcp_start(&DHCPClient);
  if (ret != NX_SUCCESS)
  {
    /* USER CODE BEGIN DHCP client start error */

    /* USER CODE END DHCP client start error */
  }

  /* wait until an IP address is ready */
  if(tx_semaphore_get(&DHCPSemaphore, NX_APP_DEFAULT_TIMEOUT) != TX_SUCCESS)
  {
    /* USER CODE BEGIN DHCPSemaphore get error */

    /* USER CODE END DHCPSemaphore get error */
  }

  /* USER CODE BEGIN Nx_App_Thread_Entry 2 */
  ULONG ip_address;
  ULONG network_mask = 0xffffffff;

  nx_ip_address_get(&NetXDuoEthIpInstance, &ip_address, &network_mask);
  printf("IP address: %lu.%lu.%lu.%lu\n",
    ip_address >> 24,
    ip_address >> 16 & 0xFF,
    ip_address >> 8 & 0xFF,
    ip_address & 0xFF);
  
  printf("Network mask: %lu.%lu.%lu.%lu\n",
    network_mask >> 24,
    network_mask >> 16 & 0xFF,
    network_mask >> 8 & 0xFF,
    network_mask & 0xFF);

  ret = nx_dns_create(&client_dns, &NetXDuoEthIpInstance, (UCHAR *)"DNS Client");
  if (ret)
  {
    printf("DNS Client create failed: %d\n", ret);
  }

  /* Initialize the cache. */
  ret = nx_dns_cache_initialize(&client_dns, local_cache, sizeof(local_cache));

  /* Check for DNS cache error. */
  if (ret)
  {
    printf("DNS cache initialize failed: %d\n", ret);
  }

  ret = nx_dns_server_add(&client_dns, DNS_SERVER_ADDRESS);
  if (ret)
  {
    printf("DNS Server add failed: %d\n", ret);
  }

  UCHAR *server_name = "www.google.com";

  /* Look up an IPv4 address over IPv4. */
  ret = nx_dns_host_by_name_get(&client_dns, server_name, &ip_address, 400);
  if (ret)
  {
    printf("DNS host by name get failed: %d\n", ret);
  }

  /* Check for DNS query error. */
  if (ret != NX_SUCCESS)
  {
      printf("DNS query failed, result = %d\n", ret);
  }
  else
  {
      printf("------------------------------------------------------\n");
      printf("Test A: \n");
      printf("IP address: %lu.%lu.%lu.%lu\n",
      ip_address >> 24,
      ip_address >> 16 & 0xFF,
      ip_address >> 8 & 0xFF,
      ip_address & 0xFF);
  }

  // ret = nx_dns_ipv4_address_by_name_get(&client_dns, 
  //                                       server_name, 
  //                                       &record_buffer[0], 
  //                                       sizeof(record_buffer), 
  //                                       &record_count, 
  //                                       400);

  // /* Check for DNS query error. */
  // if (ret)
  // {
  //   printf("DNS query failed, result = %d\n", ret);
  // }
  // else
  // {

  //     printf("------------------------------------------------------\n");
  //     printf("Test A: ");
  //     printf("record_count = %d \n", record_count);
  // }

  // /* Get the IPv4 addresses of host. */
  // for(i = 0; i < record_count; i++)
  // {
  //     ipv4_address_ptr[i] = (ULONG *)(record_buffer + i * sizeof(ULONG));
  //     printf("record %d: IP address: %lu.%lu.%lu.%lu\n", i,
  //             *ipv4_address_ptr[i] >> 24,
  //             *ipv4_address_ptr[i] >> 16 & 0xFF,
  //             *ipv4_address_ptr[i] >> 8 & 0xFF,
  //             *ipv4_address_ptr[i] & 0xFF);
  // }

  int status, packet_status;
  NX_PACKET *my_packet;
  ULONG bytes_copied;
  ULONG total_bytes_copied = 0;
  NXD_ADDRESS server_ip_addr;

  printf("http client start\n");

  /* Create an HTTP client instance.  */
  status = nx_web_http_client_create(&http_client, "My Client", &NetXDuoEthIpInstance, &NxAppPool, 1024);
  if (status)
  {
    printf("http Client create failed\n");
  }

   /* Allocate a packet.  */
  status = nx_web_http_client_request_packet_allocate(&http_client, &my_packet, NX_WAIT_FOREVER);
  if (status)
  {
    printf("http client request packet allocate failed\n");
  }

  server_ip_addr.nxd_ip_version = NX_IP_VERSION_V4;
  server_ip_addr.nxd_ip_address.v4 = ip_address;

  /* Start the GET operation on the HTTP Client "my_client." */
  status = nx_web_http_client_get_start_extended(&http_client, &server_ip_addr, NX_WEB_HTTP_SERVER_PORT,
      "/", sizeof("/") - 1,
      server_name, strlen(server_name),
      NX_NULL, 0,
      NX_NULL, 0,
      200);

  if (status)
  {
    printf("http client get failed: %d\n", status);
  }
  else
  {
    printf("http client get success: %d\n", status);
    do {
      status = nx_web_http_client_response_body_get(&http_client, &my_packet, 200);
      if (status)
      {
        printf("http client response body get failed: %d\n", status);
      }

      nx_packet_data_retrieve(my_packet, buffer, &bytes_copied);
      total_bytes_copied += bytes_copied;

      printf("%s\n", buffer);

      memset(buffer, 0, sizeof(buffer));

      packet_status = nx_packet_release(my_packet);

    } while (status == NX_SUCCESS);
  }
  printf("\n");

  printf("http get: %d, byte copited: %d\n", status, total_bytes_copied);

  status = nx_web_http_client_delete(&http_client);
  if (status)
  {
    printf("http client delete failed: %d\n", status);
  }

  /* USER CODE END Nx_App_Thread_Entry 2 */

}
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
