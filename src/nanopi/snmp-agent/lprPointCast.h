/*
 * Note: this file originally auto-generated by mib2c using
 */
#ifndef LPRPOINTCAST_H
#define LPRPOINTCAST_H

extern int dodebug;

/* function declarations */
int init_lprPointCast(int debug);
void poll_lprPointCast(void);
Netsnmp_Node_Handler handle_coil1;
Netsnmp_Node_Handler handle_coil2;
Netsnmp_Node_Handler handle_coil3;

#endif /* LPRPOINTCAST_H */