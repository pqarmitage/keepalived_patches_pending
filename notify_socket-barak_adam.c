 /***********************************************************************************************
+ * Function: vrrp_notify_thread
+ *
+ * Description:
+ * vrrp_notify thread listening over TCP socket for VRRP state change notifications from keepalived
+ *
+ ***********************************************************************************************/
+static void* vrrp_notify_thread(void* arg)
+{
+  int sockfd, client_sockfd;
+  struct sockaddr_in address;
+  fd_set readfds, testfds;
+  MSG_VRRP_NOTIFY_T vrrp_notify = {};
+  MSG_VRRP_VR_T vrrp_vr_config = {};
+  LinuxUtil_IfcType_T type;
+  int fd;
+
+  sockfd = socket(AF_INET, SOCK_STREAM, 0);
+  address.sin_family = AF_INET;
+  address.sin_addr.s_addr = htonl(INADDR_ANY);
+  address.sin_port = htons(VRRP_NOTIFY_PORT);
+  bind(sockfd, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
+
+  /* Create a connection queue and initialize a file descriptor set */
+  listen(sockfd, 10000);
+  FD_ZERO(&readfds);
+  FD_SET(sockfd, &readfds);
+
+  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
+
+  while (1) {
+    testfds = readfds;
+    select(FD_SETSIZE, &testfds, NULL, NULL, NULL);
+
+    for (fd = 0; fd < FD_SETSIZE; fd++) {
+      if (FD_ISSET(fd, &testfds)) {
+
+        if (fd == sockfd) { /* Accept a new connection request */
+          client_sockfd = accept(sockfd, NULL, NULL);
+          FD_SET(client_sockfd, &readfds);
+        }
+        else if(fd) { /* Read msg from client */
+
+          if (read(fd, &vrrp_notify, sizeof(vrrp_notify)) >= 0) {
+
+            memset(&vrrp_vr_config, 0, sizeof(vrrp_vr_config));
+            LinuxUtil_IfcNameToOpx(vrrp_notify.if_name, vrrp_vr_config.dev, IFC_MAX_LEN_IFC_NAME, &type);
+            vrrp_vr_config.state = (VRRP_STATE) vrrp_notify.state;
+            vrrp_vr_config.vrid = vrrp_notify.vrid;
+            vrrp_vr_config.field_mask = MSG_VRRP_VR_MASK_DEV | MSG_VRRP_VR_MASK_VRID | MSG_VRRP_VR_MASK_STATE;
+
+            pthread_mutex_lock(&vrrp_msg_mtx);
+            vrrp_update_state_vr(&vrrp_vr_config);
+            pthread_mutex_unlock(&vrrp_msg_mtx);
+          }
+
+          close(fd);
+          FD_CLR(fd, &readfds);
+        }
+      }
+    }
+  }
+
+  return NULL;
+}
+
