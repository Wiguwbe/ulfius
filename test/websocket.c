/* Public domain, no copyright. Use at your own risk. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <zlib.h>

#include <check.h>
#include <ulfius.h>

#define WEBSOCKET_URL "http://localhost:8378/websocket"
#define DEFAULT_PROTOCOL "proto"
#define DEFAULT_EXTENSION "ext"
#define DEFAULT_MESSAGE "message content with a few characters"
#define PORT 9275
#define PREFIX_WEBSOCKET "/websocket"
#define MESSAGE "HelloFrom"
#define MESSAGE_CLIENT "HelloFromClient"
#define MESSAGE_SERVER "HelloFromServer"
#define MESSAGE_EXT1 "Grut"
#define MESSAGE_EXT2 "Plop"
#define MESSAGE_EXT3 "Gna"
#define _U_W_BUFF_LEN 256

int websocket_client_counter, websocket_server_counter, has_deflate, has_inflate;

#ifndef U_DISABLE_WEBSOCKET
void websocket_manager_callback_empty (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_manager_user_data) {
}

void websocket_incoming_message_callback_empty (const struct _u_request * request, struct _websocket_manager * websocket_manager, const struct _websocket_message * message, void * websocket_incoming_user_data) {
}

void websocket_onclose_callback_empty (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_onclose_user_data) {
}

void websocket_onclose_callback_client (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_onclose_user_data) {
  ck_assert_ptr_ne(websocket_onclose_user_data, NULL);
}

void websocket_echo_message_callback (const struct _u_request * request,
                                         struct _websocket_manager * websocket_manager,
                                         const struct _websocket_message * last_message,
                                         void * websocket_incoming_message_user_data) {
  if (last_message->opcode == U_WEBSOCKET_OPCODE_TEXT) {
    ck_assert_int_eq(ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, last_message->data_len, last_message->data), U_OK);
  }
}

void websocket_onclose_message_callback (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_onclose_user_data) {
  o_free(websocket_onclose_user_data);
}

void websocket_manager_callback_client (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_manager_user_data) {
  int i;
  
  for (i=0; i<4; i++) {
    if (ulfius_websocket_wait_close(websocket_manager, 50) == U_WEBSOCKET_STATUS_OPEN) {
      if (i%2) {
        ck_assert_int_eq(ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, o_strlen(DEFAULT_MESSAGE), DEFAULT_MESSAGE), U_OK);
      } else {
        ck_assert_int_eq(ulfius_websocket_send_fragmented_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, o_strlen(DEFAULT_MESSAGE), DEFAULT_MESSAGE, (o_strlen(DEFAULT_MESSAGE)/4)), U_OK);
      }
    }
  }
}

void websocket_extension_callback_client (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_manager_user_data) {
  ck_assert_int_eq(ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, o_strlen(MESSAGE_CLIENT), MESSAGE_CLIENT), U_OK);
  y_log_message(Y_LOG_LEVEL_DEBUG, "client message sent");
  if (ulfius_websocket_wait_close(websocket_manager, 50) == U_WEBSOCKET_STATUS_OPEN) {
    ck_assert_int_eq(ulfius_websocket_send_close_signal(websocket_manager), U_OK);
  }
}

void websocket_incoming_message_callback_client (const struct _u_request * request, struct _websocket_manager * websocket_manager, const struct _websocket_message * message, void * websocket_incoming_user_data) {
  ck_assert_int_eq(0, o_strncmp(message->data, DEFAULT_MESSAGE, message->data_len));
}

void websocket_manager_extension_callback (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_manager_user_data) {
  ck_assert_int_eq(ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, o_strlen(MESSAGE_SERVER), MESSAGE_SERVER), U_OK);
  y_log_message(Y_LOG_LEVEL_DEBUG, "server message sent");
  if (ulfius_websocket_wait_close(websocket_manager, 50) == U_WEBSOCKET_STATUS_OPEN) {
    ck_assert_int_eq(ulfius_websocket_send_close_signal(websocket_manager), U_OK);
  }
}

void websocket_manager_extension_deflate_callback (const struct _u_request * request, struct _websocket_manager * websocket_manager, void * websocket_manager_user_data) {
  ck_assert_int_eq(ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, o_strlen(MESSAGE_SERVER), MESSAGE_SERVER), U_OK);
  y_log_message(Y_LOG_LEVEL_DEBUG, "server message sent");
  if (ulfius_websocket_wait_close(websocket_manager, 50) == U_WEBSOCKET_STATUS_OPEN) {
    ck_assert_int_eq(ulfius_websocket_send_close_signal(websocket_manager), U_OK);
  }
}

void websocket_incoming_extension_callback (const struct _u_request * request,
                                            struct _websocket_manager * websocket_manager,
                                            const struct _websocket_message * last_message,
                                            void * user_data) {
  int * rsv = (int *)user_data;
  
  ck_assert_ptr_ne(NULL, o_strnstr(last_message->data, MESSAGE, last_message->data_len));
  if ((*rsv)&U_WEBSOCKET_RSV1) {
    ck_assert_int_ne(last_message->rsv&U_WEBSOCKET_RSV1, 0);
    ck_assert_ptr_ne(NULL, o_strnstr(last_message->data, MESSAGE_EXT1, last_message->data_len));
  }
  if ((*rsv)&U_WEBSOCKET_RSV2) {
    ck_assert_int_ne(last_message->rsv&U_WEBSOCKET_RSV2, 0);
    ck_assert_ptr_ne(NULL, o_strnstr(last_message->data, MESSAGE_EXT2, last_message->data_len));
  }
  if ((*rsv)&U_WEBSOCKET_RSV3) {
    ck_assert_int_ne(last_message->rsv&U_WEBSOCKET_RSV3, 0);
    ck_assert_ptr_ne(NULL, o_strnstr(last_message->data, MESSAGE_EXT3, last_message->data_len));
  }
}

void websocket_incoming_extension_deflate_client_callback (const struct _u_request * request,
                                                    struct _websocket_manager * websocket_manager,
                                                    const struct _websocket_message * last_message,
                                                    void * user_data) {
  ck_assert_int_eq(0, o_strncmp(last_message->data, MESSAGE_SERVER, last_message->data_len));
  ck_assert_int_ne(last_message->rsv&U_WEBSOCKET_RSV1, 0);
}

void websocket_incoming_extension_deflate_server_callback (const struct _u_request * request,
                                                    struct _websocket_manager * websocket_manager,
                                                    const struct _websocket_message * last_message,
                                                    void * user_data) {
  ck_assert_int_eq(0, o_strncmp(last_message->data, MESSAGE_CLIENT, last_message->data_len));
  ck_assert_int_ne(last_message->rsv&U_WEBSOCKET_RSV1, 0);
}

int websocket_extension1_message_out_perform(const uint8_t opcode, uint8_t * rsv, const uint64_t data_len_in, const char * data_in, uint64_t * data_len_out, char ** data_out, const uint64_t fragment_len, void * user_data) {
  ck_assert_int_eq(opcode, U_WEBSOCKET_OPCODE_TEXT);
  if (user_data == NULL) {
    websocket_server_counter++;
  } else {
    websocket_client_counter++;
  }
  *data_out = msprintf("%.*s%s", data_len_in, data_in, MESSAGE_EXT1);
  *data_len_out = o_strlen(*data_out);
  *rsv |= U_WEBSOCKET_RSV1;
  y_log_message(Y_LOG_LEVEL_DEBUG, "inside websocket_extension1_message_out_perform");
  return U_OK;
}

int websocket_extension1_message_in_perform(const uint8_t opcode, uint8_t rsv, const uint64_t data_len_in, const char * data_in, uint64_t * data_len_out, char ** data_out, const uint64_t fragment_len, void * user_data) {
  ck_assert_int_eq(opcode, U_WEBSOCKET_OPCODE_TEXT);
  ck_assert_int_ne(rsv & U_WEBSOCKET_RSV1, 0);
  if (user_data == NULL) {
    websocket_server_counter++;
  } else {
    websocket_client_counter++;
  }
  *data_out = msprintf("%.*s%s", data_len_in, data_in, MESSAGE_EXT1);
  *data_len_out = o_strlen(*data_out);
  y_log_message(Y_LOG_LEVEL_DEBUG, "inside websocket_extension1_message_in_perform");
  return U_OK;
}

int websocket_extension2_message_out_perform(const uint8_t opcode, uint8_t * rsv, const uint64_t data_len_in, const char * data_in, uint64_t * data_len_out, char ** data_out, const uint64_t fragment_len, void * user_data) {
  ck_assert_int_eq(opcode, U_WEBSOCKET_OPCODE_TEXT);
  if (user_data == NULL) {
    websocket_server_counter++;
  } else {
    websocket_client_counter++;
  }
  *data_out = msprintf("%.*s%s", data_len_in, data_in, MESSAGE_EXT2);
  *data_len_out = o_strlen(*data_out);
  *rsv |= U_WEBSOCKET_RSV2;
  y_log_message(Y_LOG_LEVEL_DEBUG, "inside websocket_extension2_message_out_perform");
  return U_OK;
}

int websocket_extension2_message_in_perform(const uint8_t opcode, uint8_t rsv, const uint64_t data_len_in, const char * data_in, uint64_t * data_len_out, char ** data_out, const uint64_t fragment_len, void * user_data) {
  ck_assert_int_eq(opcode, U_WEBSOCKET_OPCODE_TEXT);
  ck_assert_int_ne(rsv & U_WEBSOCKET_RSV2, 0);
  if (user_data == NULL) {
    websocket_server_counter++;
  } else {
    websocket_client_counter++;
  }
  *data_out = msprintf("%.*s%s", data_len_in, data_in, MESSAGE_EXT2);
  *data_len_out = o_strlen(*data_out);
  y_log_message(Y_LOG_LEVEL_DEBUG, "inside websocket_extension2_message_in_perform");
  return U_OK;
}

int callback_websocket (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int ret;
  char * websocket_allocated_data = o_strdup("grut");
  
  ret = ulfius_set_websocket_response(response, NULL, NULL, NULL, NULL, &websocket_echo_message_callback, websocket_allocated_data, &websocket_onclose_message_callback, websocket_allocated_data);
  ck_assert_int_eq(ret, U_OK);
  return (ret == U_OK)?U_CALLBACK_CONTINUE:U_CALLBACK_ERROR;
}

int callback_websocket_onclose (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int ret;
  
  ret = ulfius_set_websocket_response(response, NULL, NULL, NULL, NULL, &websocket_echo_message_callback, NULL, NULL, NULL);
  ck_assert_int_eq(ret, U_OK);
  return (ret == U_OK)?U_CALLBACK_CONTINUE:U_CALLBACK_ERROR;
}

int callback_websocket_extension (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ck_assert_int_eq(ulfius_set_websocket_response(response, NULL, NULL, &websocket_manager_extension_callback, NULL, &websocket_incoming_extension_callback, user_data, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_extension_message_perform(response, MESSAGE_EXT1, &websocket_extension1_message_out_perform, NULL, &websocket_extension1_message_in_perform, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_extension_message_perform(response, MESSAGE_EXT2, &websocket_extension2_message_out_perform, NULL, &websocket_extension2_message_in_perform, NULL, NULL, NULL), U_OK);
  return U_CALLBACK_CONTINUE;
}

int websocket_extension_server_match_ext1(const char * extension_server, char ** extension_client, void * user_data) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "check match server extension "MESSAGE_EXT1" with extension '%s'", extension_server);
  if (0 == o_strncmp(extension_server, MESSAGE_EXT1, o_strlen(MESSAGE_EXT1))) {
    *extension_client = o_strdup(MESSAGE_EXT1);
    return U_OK;
  } else {
    return U_ERROR;
  }
}

int websocket_extension_server_match_ext2(const char * extension_server, char ** extension_client, void * user_data) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "check match server extension "MESSAGE_EXT2" with extension '%s'", extension_server);
  if (0 == o_strncmp(extension_server, MESSAGE_EXT2, o_strlen(MESSAGE_EXT2))) {
    *extension_client = o_strdup(MESSAGE_EXT2);
    return U_OK;
  } else {
    return U_ERROR;
  }
}

int websocket_extension_client_match_ext1(const char * extension_server, void * user_data) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "check match client extension "MESSAGE_EXT1" with extension '%s'", extension_server);
  if (0 == o_strncmp(extension_server, MESSAGE_EXT1, o_strlen(MESSAGE_EXT1))) {
    return U_OK;
  } else {
    return U_ERROR;
  }
}

int websocket_extension_client_match_ext2(const char * extension_server, void * user_data) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "check match client extension "MESSAGE_EXT2" with extension '%s'", extension_server);
  if (0 == o_strncmp(extension_server, MESSAGE_EXT2, o_strlen(MESSAGE_EXT2))) {
    return U_OK;
  } else {
    return U_ERROR;
  }
}

int callback_websocket_extension_match (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ck_assert_int_eq(ulfius_set_websocket_response(response, NULL, NULL, &websocket_manager_extension_callback, NULL, &websocket_incoming_extension_callback, user_data, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_extension_message_perform(response, MESSAGE_EXT1, &websocket_extension1_message_out_perform, NULL, &websocket_extension1_message_in_perform, NULL, &websocket_extension_server_match_ext1, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_extension_message_perform(response, MESSAGE_EXT2, &websocket_extension2_message_out_perform, NULL, &websocket_extension2_message_in_perform, NULL, &websocket_extension_server_match_ext2, NULL), U_OK);
  return U_CALLBACK_CONTINUE;
}

int websocket_extension_message_out_deflate_test(const uint8_t opcode,
                                                uint8_t * rsv,
                                                const uint64_t data_len_in,
                                                const char * data_in,
                                                uint64_t * data_len_out,
                                                char ** data_out,
                                                const uint64_t fragment_len,
                                                void * user_data) {
  z_stream defstream;
  unsigned char data_out_pre[_U_W_BUFF_LEN] = {0};
  int ret;
  (void)opcode;
  (void)fragment_len;
  (void)user_data;
  
  if (data_len_in) {
      defstream.zalloc = Z_NULL;
      defstream.zfree = Z_NULL;
      defstream.opaque = Z_NULL;
      defstream.avail_in = (uInt)data_len_in;
      defstream.next_in = (Bytef *)data_in;
      defstream.avail_out = (uInt)_U_W_BUFF_LEN;
      defstream.next_out = (Bytef *)data_out_pre;
      
      if (deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) == Z_OK) {
        ret = U_OK;
        has_deflate = 1;
        do {
          switch (deflate(&defstream, Z_SYNC_FLUSH)) {
            case Z_OK:
            case Z_STREAM_END:
            case Z_BUF_ERROR:
              if ((*data_out = o_realloc(*data_out, (*data_len_out + defstream.total_out + 1))) != NULL) {
                memcpy((*data_out)+(*data_len_out), data_out_pre, defstream.total_out);
                *data_len_out += defstream.total_out;
              } else {
                y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_out_deflate_test - Error o_realloc for data_out");
                ret = U_ERROR;
              }
              break;
            default:
              y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_out_deflate_test - Error deflate");
              o_free(*data_out);
              ret = U_ERROR;
              break;
          }
        } while(defstream.total_out == 0 && ret == U_OK);
        deflateEnd(&defstream);
        
        if ((*(unsigned char **)data_out)[*data_len_out-1] == 0xff && (*(unsigned char **)data_out)[*data_len_out-2] == 0xff && (*(unsigned char **)data_out)[*data_len_out-3] == 0x00 && (*(unsigned char **)data_out)[*data_len_out-4] == 0x00) {
          *data_len_out -= 4;
        } else {
          (*(unsigned char **)data_out)[defstream.total_out] = '\0';
          (*data_len_out)++;
        }
        *rsv |= U_WEBSOCKET_RSV1;
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_out_deflate_test - Error deflateInit");
        ret = U_ERROR;
      }
  } else {
    *data_len_out = 0;
    *rsv |= U_WEBSOCKET_RSV1;
    ret = U_OK;
  }
  ck_assert_int_eq(U_OK, ret);
  return ret;
}

int websocket_extension_message_in_inflate_test(const uint8_t opcode,
                                               uint8_t rsv,
                                               const uint64_t data_len_in,
                                               const char * data_in,
                                               uint64_t * data_len_out,
                                               char ** data_out,
                                               const uint64_t fragment_len,
                                               void * user_data) {
  z_stream infstream;
  unsigned char inf_out[_U_W_BUFF_LEN] = {0}, * data_in_suffix;
  unsigned char suffix[4] = {0x00, 0x00, 0xff, 0xff};
  int ret;
  (void)opcode;
  (void)fragment_len;
  (void)user_data;

  if (data_len_in && rsv&U_WEBSOCKET_RSV1) {
    if ((data_in_suffix = o_malloc(data_len_in+4)) != NULL) {
      memcpy(data_in_suffix, data_in, data_len_in);
      memcpy(data_in_suffix+data_len_in, suffix, 4);
      infstream.zalloc = Z_NULL;
      infstream.zfree = Z_NULL;
      infstream.opaque = Z_NULL;
      infstream.avail_in = (uInt)data_len_in+4;
      infstream.next_in = (Bytef *)data_in_suffix;
      infstream.avail_out = _U_W_BUFF_LEN;
      infstream.next_out = (Bytef *)inf_out;
      
      if (inflateInit2(&infstream, -15) == Z_OK) {
        ret = U_OK;
        has_inflate = 1;
        do {
          int res;
          switch ((res = inflate(&infstream, Z_SYNC_FLUSH))) {
            case Z_OK:
            case Z_STREAM_END:
            case Z_BUF_ERROR:
              if ((*data_out = o_realloc(*data_out, (*data_len_out + infstream.total_out))) != NULL) {
                memcpy((*data_out)+(*data_len_out), inf_out, infstream.total_out);
                *data_len_out += infstream.total_out;
              } else {
                y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_in_inflate_test - Error o_realloc for data_out");
                ret = U_ERROR;
              }
              break;
            default:
              y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_in_inflate_test - Error inflate %d", res);
              o_free(*data_out);
              ret = U_ERROR;
              break;
          }
        } while (infstream.total_out == 0 && ret == U_OK);
        inflateEnd(&infstream);
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_in_inflate_test - Error inflateInit");
        ret = U_ERROR;
      }
      o_free(data_in_suffix);
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "websocket_extension_message_in_inflate_test - Error allocating resources for data_in_suffix");
      ret = U_ERROR;
    }
  } else {
    *data_len_out = 0;
    ret = U_OK;
  }
  ck_assert_int_eq(U_OK, ret);
  return ret;
}

int callback_websocket_extension_deflate (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ck_assert_int_eq(ulfius_set_websocket_response(response, NULL, NULL, &websocket_manager_extension_deflate_callback, NULL, &websocket_incoming_extension_deflate_server_callback, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_compression_extension(response, user_data==NULL?0:1), U_OK);
  return U_CALLBACK_CONTINUE;
}

int callback_websocket_extension_deflate_debug (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ck_assert_int_eq(ulfius_set_websocket_response(response, NULL, NULL, &websocket_manager_extension_deflate_callback, NULL, &websocket_incoming_extension_deflate_server_callback, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_extension_message_perform(response, "permessage-deflate", &websocket_extension_message_out_deflate_test, NULL, &websocket_extension_message_in_inflate_test, NULL, NULL, NULL), U_OK);
  return U_CALLBACK_CONTINUE;
}

START_TEST(test_ulfius_websocket_set_websocket_response)
{
  struct _u_response response;
  ulfius_init_response(&response);
  
  ck_assert_int_eq(ulfius_set_websocket_response(NULL, DEFAULT_PROTOCOL, DEFAULT_EXTENSION, &websocket_manager_callback_empty, NULL, NULL, NULL, NULL, NULL), U_ERROR_PARAMS);
  ck_assert_int_eq(ulfius_set_websocket_response(&response, DEFAULT_PROTOCOL, DEFAULT_EXTENSION, NULL, NULL, NULL, NULL, NULL, NULL), U_ERROR_PARAMS);
  ck_assert_int_eq(ulfius_set_websocket_response(&response, DEFAULT_PROTOCOL, DEFAULT_EXTENSION, NULL, NULL, NULL, NULL, &websocket_onclose_callback_empty, NULL), U_ERROR_PARAMS);
  ck_assert_int_eq(ulfius_set_websocket_response(&response, DEFAULT_PROTOCOL, DEFAULT_EXTENSION, &websocket_manager_callback_empty, NULL, NULL, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_set_websocket_response(&response, DEFAULT_PROTOCOL, DEFAULT_EXTENSION, NULL, NULL, &websocket_incoming_message_callback_empty, NULL, NULL, NULL), U_OK);
  
  ulfius_clean_response(&response);
}
END_TEST

START_TEST(test_ulfius_websocket_set_websocket_request)
{
  struct _u_request request;
  ulfius_init_request(&request);
  
  ck_assert_int_eq(ulfius_set_websocket_request(NULL, WEBSOCKET_URL, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_ERROR);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, NULL, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_ERROR);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, WEBSOCKET_URL, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, WEBSOCKET_URL, NULL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, WEBSOCKET_URL, DEFAULT_PROTOCOL, NULL), U_OK);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, WEBSOCKET_URL, NULL, NULL), U_OK);
  
  ulfius_clean_request(&request);
}
END_TEST

START_TEST(test_ulfius_websocket_open_websocket_client_connection_error)
{
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler;
  ulfius_init_request(&request);
  ulfius_init_response(&response);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, WEBSOCKET_URL, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  
  ck_assert_int_eq(ulfius_open_websocket_client_connection(NULL, &websocket_manager_callback_empty, NULL, NULL, NULL, NULL, NULL, &websocket_client_handler, &response), U_ERROR_PARAMS);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, NULL, NULL, NULL, NULL, NULL, NULL, &websocket_client_handler, &response), U_ERROR_PARAMS);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, NULL, NULL, NULL, NULL, &websocket_onclose_callback_empty, NULL, &websocket_client_handler, &response), U_ERROR_PARAMS);
  
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
}
END_TEST

START_TEST(test_ulfius_websocket_client)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64], * allocated_data = o_strdup("plop");

  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket, allocated_data), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);
  
  // Test correct websocket connection on correct websocket service
  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_manager_callback_client, NULL, &websocket_incoming_message_callback_client, NULL, websocket_onclose_callback_client, allocated_data, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  // Test incorrect websocket connection on correct websocket service
  ulfius_init_request(&request);
  ulfius_init_response(&response);
  request.http_verb = o_strdup("GET");
  request.http_url = o_strdup(url);
  ck_assert_int_eq(ulfius_send_http_request(&request, &response), U_ERROR_LIBCURL); // On a websocket connection attempt, libcurl return 'Unsupported protocol'
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  // Test incorrect websocket connection on correct websocket service
  ulfius_init_request(&request);
  ulfius_init_response(&response);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_manager_callback_client, NULL, &websocket_incoming_message_callback_client, NULL, websocket_onclose_callback_client, allocated_data, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);
  ulfius_clean_instance(&instance);
  o_free(allocated_data);
}
END_TEST

START_TEST(test_ulfius_websocket_client_no_onclose)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];
  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_onclose, NULL), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);
  
  // Test correct websocket connection on correct websocket service
  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_manager_callback_client, NULL, &websocket_incoming_message_callback_client, NULL, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  // Test incorrect websocket connection on correct websocket service
  ulfius_init_request(&request);
  ulfius_init_response(&response);
  request.http_verb = o_strdup("GET");
  request.http_url = o_strdup(url);
  ck_assert_int_eq(ulfius_send_http_request(&request, &response), U_ERROR_LIBCURL); // On a websocket connection attempt, libcurl return 'Unsupported protocol'
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  // Test incorrect websocket connection on correct websocket service
  ulfius_init_request(&request);
  ulfius_init_response(&response);
  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, DEFAULT_PROTOCOL, DEFAULT_EXTENSION), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_manager_callback_client, NULL, &websocket_incoming_message_callback_client, NULL, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);
  ulfius_clean_instance(&instance);
}
END_TEST

START_TEST(test_ulfius_websocket_extension_no_match_function)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];
  int user_data = U_WEBSOCKET_RSV1;

  websocket_server_counter = 0;
  websocket_client_counter = 0;

  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_extension, &user_data), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);

  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, NULL, MESSAGE_EXT1), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, MESSAGE_EXT1, &websocket_extension1_message_out_perform, (void *)1, &websocket_extension1_message_in_perform, (void *)1, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, MESSAGE_EXT2, &websocket_extension2_message_out_perform, (void *)1, &websocket_extension2_message_in_perform, (void *)1, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_extension_callback_client, NULL, &websocket_incoming_extension_callback, &user_data, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);
  
  ck_assert_int_eq(websocket_server_counter, 2);
  ck_assert_int_eq(websocket_client_counter, 2);

  ulfius_clean_instance(&instance);
}
END_TEST

START_TEST(test_ulfius_websocket_extension_match_function)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];
  int user_data = U_WEBSOCKET_RSV1|U_WEBSOCKET_RSV2;

  websocket_server_counter = 0;
  websocket_client_counter = 0;

  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_extension_match, &user_data), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);

  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, NULL, MESSAGE_EXT1 ", " MESSAGE_EXT2), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, MESSAGE_EXT1, &websocket_extension1_message_out_perform, (void *)1, &websocket_extension1_message_in_perform, (void *)1, &websocket_extension_client_match_ext1, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, MESSAGE_EXT2, &websocket_extension2_message_out_perform, (void *)1, &websocket_extension2_message_in_perform, (void *)1, &websocket_extension_client_match_ext2, NULL), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_extension_callback_client, NULL, &websocket_incoming_extension_callback, &user_data, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);

  ck_assert_int_eq(websocket_server_counter, 4);
  ck_assert_int_eq(websocket_client_counter, 4);
  
  ulfius_clean_instance(&instance);
}
END_TEST

START_TEST(test_ulfius_websocket_extension_deflate_server_force_no_parameters)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];

  has_deflate = 0;
  has_inflate = 0;
  
  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_extension_deflate, NULL), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);

  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, NULL, "permessage-deflate"), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, "permessage-deflate", &websocket_extension_message_out_deflate_test, NULL, &websocket_extension_message_in_inflate_test, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_extension_callback_client, NULL, &websocket_incoming_extension_deflate_client_callback, NULL, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);

  ck_assert_int_eq(has_deflate, 1);
  ck_assert_int_eq(has_inflate, 1);
  
  ulfius_clean_instance(&instance);
}
END_TEST

START_TEST(test_ulfius_websocket_extension_deflate_server_no_force_no_parameters)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];

  has_deflate = 0;
  has_inflate = 0;
  
  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_extension_deflate, (void *)0x1), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);

  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, NULL, "permessage-deflate; client_max_window_bits; server_max_window_bits=10"), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_extension_message_perform(&websocket_client_handler, "permessage-deflate", &websocket_extension_message_out_deflate_test, NULL, &websocket_extension_message_in_inflate_test, NULL, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_extension_callback_client, NULL, &websocket_incoming_extension_deflate_client_callback, NULL, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);

  ck_assert_int_eq(has_deflate, 1);
  ck_assert_int_eq(has_inflate, 1);
  
  ulfius_clean_instance(&instance);
}
END_TEST

START_TEST(test_ulfius_websocket_extension_deflate_client)
{
  struct _u_instance instance;
  struct _u_request request;
  struct _u_response response;
  struct _websocket_client_handler websocket_client_handler = {NULL, NULL};
  char url[64];

  has_deflate = 0;
  has_inflate = 0;
  
  ck_assert_int_eq(ulfius_init_instance(&instance, PORT, NULL, NULL), U_OK);
  ck_assert_int_eq(ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_WEBSOCKET, NULL, 0, &callback_websocket_extension_deflate_debug, NULL), U_OK);
  ck_assert_int_eq(ulfius_start_framework(&instance), U_OK);

  ulfius_init_request(&request);
  ulfius_init_response(&response);
  sprintf(url, "ws://localhost:%d/%s", PORT, PREFIX_WEBSOCKET);

  ck_assert_int_eq(ulfius_set_websocket_request(&request, url, NULL, "permessage-deflate"), U_OK);
  ck_assert_int_eq(ulfius_add_websocket_client_compression_extension(&websocket_client_handler), U_OK);
  ck_assert_int_eq(ulfius_open_websocket_client_connection(&request, &websocket_extension_callback_client, NULL, &websocket_incoming_extension_deflate_client_callback, NULL, NULL, NULL, &websocket_client_handler, &response), U_OK);
  ck_assert_int_eq(ulfius_websocket_client_connection_wait_close(&websocket_client_handler, 0), U_WEBSOCKET_STATUS_CLOSE);
  ulfius_clean_request(&request);
  ulfius_clean_response(&response);
  ck_assert_int_eq(ulfius_stop_framework(&instance), U_OK);

  ck_assert_int_eq(has_deflate, 1);
  ck_assert_int_eq(has_inflate, 1);
  
  ulfius_clean_instance(&instance);
}
END_TEST

#endif

static Suite *ulfius_suite(void)
{
	Suite *s;
	TCase *tc_websocket;

	s = suite_create("Ulfius websocket functions tests");
	tc_websocket = tcase_create("test_ulfius_websocket");
#ifndef U_DISABLE_WEBSOCKET
  tcase_add_test(tc_websocket, test_ulfius_websocket_set_websocket_response);
	tcase_add_test(tc_websocket, test_ulfius_websocket_set_websocket_request);
	tcase_add_test(tc_websocket, test_ulfius_websocket_open_websocket_client_connection_error);
	tcase_add_test(tc_websocket, test_ulfius_websocket_client);
	tcase_add_test(tc_websocket, test_ulfius_websocket_client_no_onclose);
	tcase_add_test(tc_websocket, test_ulfius_websocket_extension_no_match_function);
	tcase_add_test(tc_websocket, test_ulfius_websocket_extension_match_function);
	tcase_add_test(tc_websocket, test_ulfius_websocket_extension_deflate_server_force_no_parameters);
	tcase_add_test(tc_websocket, test_ulfius_websocket_extension_deflate_server_no_force_no_parameters);
	tcase_add_test(tc_websocket, test_ulfius_websocket_extension_deflate_client);
#endif
	tcase_set_timeout(tc_websocket, 30);
	suite_add_tcase(s, tc_websocket);

	return s;
}

int main(int argc, char *argv[])
{
  int number_failed;
  Suite *s;
  SRunner *sr;
  //y_init_logs("Ulfius", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting Ulfius websocket tests");
  ulfius_global_init();
  s = ulfius_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  
  ulfius_global_close();
  //y_close_logs();
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
