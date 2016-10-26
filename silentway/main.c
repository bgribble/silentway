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

// "port type" is the ES device that this port 
// is connected to 
typedef enum {
    PORT_NC, 
    PORT_GT8,
    PORT_MD8,
    PORT_CV4,
    PORT_CV8
} sway_port_type_t;


// "stream type" is the kind of musical data that's 
// being sent as input to this program for the indicated 
// port 
typedef enum {
    STREAM_SIGNAL,    // audio or CV sent as audio
    STREAM_MIDI_CV,   // MIDI notes or CC converted to CV
    STREAM_MIDI_GATE, // MIDI events converted to gates 
    STREAM_MIDI_MIDI  // MIDI events encoded for the MD8 
} sway_stream_type_t;

typedef enum {
    IFACE_ES5,
    IFACE_ES4,
    IFACE_ES40
} sway_iface_type_t;

typedef struct { 
    jack_client_t * client;
    
    GArray * input_ports;
    GArray * output_ports; 

    sway_iface_type_t cfg_iface;

} sway_ctxt;

typedef struct {
    sway_stream_type_t stype;
    double cal_offset;
    double cal_scale; 
} sway_audio_stream_t; 

typedef struct { 
    sway_stream_type_t stype;
    int midi_chan;
    int midi_note;
    int midi_cc;
    int cal_transpose;
    double cal_offset;
    double cal_scale;
} sway_midi_stream_t;


typedef union {
    sway_stream_type_t stype;
    sway_audio_stream_t audio;
    sway_midi_stream_t midi;
} sway_stream_t;


typedef struct { 
    sway_port_type_t ptype;
    sway_stream_t * streams[8];
} sway_port;


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


static int
init_ctxt(sway_ctxt * ctxt, GKeyFile * config) 
{
    const char * iface = g_key_file_get_value(config, "silentway", "interface", NULL);
    
    // interface type 
    if (!strcmp(iface, "es4")) {
        ctxt->cfg_iface = IFACE_ES4;
        return 1;
    }
    else if (!strcmp(iface, "es5")) {
        ctxt->cfg_iface = IFACE_ES5;
        return 1;
    }
    else if (!strcmp(iface, "es40")) { 
        ctxt->cfg_iface = IFACE_ES40;
        return 1;
    }
   
    if (iface) { 
        printf("[config] Error in config file.. unrecognized interface '%s'\n",
               iface);
        return 0;
    }
    else { 
        printf("[config] Error in config file.. no interface defined\n");
        return 0;
    }
}

static int
init_port(sway_ctxt * ctxt, GKeyFile * config, const gchar * port) {
    
    sway_port_t * port = g_malloc(sizeof(sway_port));
    const char * device = g_key_file_get_value(config, port, "device", NULL);



}


static sway_ctxt * 
sway_jack_startup(char * client_name, GKeyFile config) 
{
    sway_ctxt * ctxt;
    jack_status_t status;
    jack_port_t * port;
    int i;

    char namebuf[16];

    ctxt = g_malloc(sizeof(sway_ctxt));
    init_ctxt(ctxt, config);

    if ((ctxt->client = jack_client_open(client_name, JackNullOption, &status, NULL)) == 0) {
        fprintf (stderr, "jack_client_open() failed.");
        return NULL;
    }
    
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

    /* always 2 audio outputs */
    for(i=0; i < 2; i++) {
        snprintf(namebuf, 16, "out_%d", i);
        port = jack_port_register(
                ctxt->client, namebuf, JACK_DEFAULT_AUDIO_TYPE, 
                JackPortIsOutput, i
        );
        g_array_append_val(ctxt->output_ports, port);
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


