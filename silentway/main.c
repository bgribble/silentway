#include <glib.h>
#include <jack/jack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <argp.h>


typedef enum {
    NC_PORT, 
    GT8_PORT,
    MD8_PORT,
    CV4_PORT,
    CV8_PORT
} sway_port_type_t;

typedef struct { 
    jack_client_t * client;
    
    GArray * input_ports;
    GArray * output_ports; 
} sway_ctxt;

typedef struct { 

    sway_port_type_t ports[6];
} sway_ports;


static int
process_cb (jack_nframes_t nframes, void * ctxt_arg)
{
    sway_ctxt * ctxt = (sway_ctxt *)ctxt_arg;

    /* work ! */
    printf("-- process callback --");

    return 0;
}

    
static int
shutdown_cb(void * arg) 
{
    printf("Called shutdown_cb, exiting.\n");
    exit(-1);
}


static sway_ctxt * 
sway_jack_startup(char * client_name, sway_ports * portinfo) 
{
    sway_ctxt * ctxt;
    jack_status_t status;
    jack_port_t * port;
    int i;

    char namebuf[16];

    ctxt = malloc(sizeof(sway_ctxt));

    if ((ctxt->client = jack_client_open(client_name, JackNullOption, &status, NULL)) == 0) {
        fprintf (stderr, "jack_client_open() failed.");
        return NULL;
    }
    
    /* set up the process callback */ 
    jack_set_process_callback(ctxt->client, process_cb, ctxt);
    
    if (num_inputs > 0) {
        ctxt->input_ports = g_array_new(TRUE, TRUE, sizeof(jack_port_t *));

        for(i=0; i<num_inputs; i++) {
            snprintf(namebuf, 16, "in_%d", i);
            port = jack_port_register (ctxt->client, namebuf, JACK_DEFAULT_AUDIO_TYPE, 
                                       JackPortIsInput, i);
            g_array_append_val(ctxt->input_ports, port);
        }
    }

    /* 2 outputs */
    if (num_outputs > 0) {
        for(i=0; i < 2; i++) {
            snprintf(namebuf, 16, "out_%d", i);
            port = jack_port_register(
                    ctxt->client, namebuf, JACK_DEFAULT_AUDIO_TYPE, 
                    JackPortIsOutput, i
            );
            g_array_append_val(ctxt->output_ports, port);
        }

    }
    
    /* tell the JACK server that we are ready to roll */
    if (jack_activate (ctxt->client)) {
        fprintf (stderr, "cannot activate client");
        return NULL;
    }
    else {
        ctxt->activated = 1;
    }

    return ctxt;

}
const char * argp_program_name = "silentway";
const char * argp_program_version = "silentway v0.01";
const char * argp_program_bug_address = "grib@billgribble.com";

static char doc [] = "A command-line Silent Way translator";

int
main(int argc, char ** argv) 
{
    /* parse command line args */
    static struct argp argp = { 0, 0, 0, doc };
    argp_parse(&argp, argc, argv, 0, 0, 0);

    const char * conf_file = "sway.conf";

    /* read config file */ 
    GKeyFile * config = sway_load_config(conf_file);

    /* initialize JACK */

    sway_ctxt * ctxt = sway_jack_startup("silentway", config);

    /* wait forever */
    sleep(-1);

}


