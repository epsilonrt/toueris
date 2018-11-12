#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include "lprPointCast.h"

#ifndef BACKGROUND
#define BACKGROUND 0
#endif

#ifndef SYSLOG
#define SYSLOG 0
#endif

void init_usmUser (void);
void init_vacm_vars (void);

static int keep_running;

RETSIGTYPE
stop_server (int a) {
  keep_running = 0;
}

int
main (int argc, char **argv) {
  int agentx_subagent = 1; /* change this if you want to be a SNMP master agent */
  int background = BACKGROUND; /* change this if you want to run in the background */
  int syslog = SYSLOG; /* change this if you want to use syslog */

  /* print log errors to syslog or stderr */
  if (syslog) {
    snmp_enable_calllog();
  }
  else {
    snmp_enable_stderrlog();
  }

  /* we're an agentx subagent? */
  if (agentx_subagent) {
    /* make us a agentx client. */
    netsnmp_ds_set_boolean (NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    netsnmp_ds_set_string (NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET,
                           "tcp:localhost:705");
  }

  /* run in background, if requested */
  if (background && netsnmp_daemonize (1, !syslog)) {
    exit (1);
  }

  /* initialize tcpip, if necessary */
  SOCK_STARTUP;

  /* initialize the agent library */
  init_agent ("pointcast-agent");

  /* initialize mib code here */
  if (init_lprPointCast(! background) != SNMP_ERR_NOERROR) {
    
    exit(EXIT_FAILURE);
  }

  /* initialize vacm/usm access control  */
  if (!agentx_subagent) {
    init_vacm_vars();
    init_usmUser();
  }

  /* pointcast-agent will be used to read pointcast-agent.conf files. */
  init_snmp ("pointcast-agent");

  /* If we're going to be a snmp master agent, initial the ports */
  if (!agentx_subagent) {
    init_master_agent();  /* open the port to listen on (defaults to udp:161) */
  }

  /* In case we recevie a request to stop (kill -TERM or kill -INT) */
  keep_running = 1;
  signal (SIGTERM, stop_server);
  signal (SIGINT, stop_server);

  snmp_log (LOG_INFO, "pointcast-agent is up and running.\n");

  /* your main loop here... */
  while (keep_running) {
    /* if you use select(), see snmp_select_info() in snmp_api(3) */
    /*     --- OR ---  */
    agent_check_and_process (0); /* 0 == don't block */
    poll_lprPointCast();
  }

  /* at shutdown time */
  snmp_shutdown ("pointcast-agent");
  SOCK_CLEANUP;

  return 0;
}
