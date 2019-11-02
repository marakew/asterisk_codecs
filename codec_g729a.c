
//asterisk 17.x

#include "asterisk.h"

#include "asterisk/translate.h"
#include "asterisk/config.h"
#include "asterisk/module.h"
#include "asterisk/utils.h"
#include "asterisk/linkedlists.h"

#include "g729a/typedef.h"
#include "g729a/ld8a.h"
#include "g729a/g729a.h"

#include "asterisk/slin.h"

#define SLIN_FRAME_LEN 160
#define G729_FRAME_LEN 10
#define G729_SAMPLES 80
#define BUFFER_SAMPLES 8000

#define FRAME_DATA data.ptr
#define OUTBUF_SLIN outbuf.i16
#define OUTBUF_G729 outbuf.uc

//------------ DECODER

struct g729_decoder_pvt {
	struct decoder_state state;
};

static int g729tolin_new(struct ast_trans_pvt *pvt)
{
	g729_decoder_pvt *tmp = pvt->pvt;
	g729a_decoder_init(&tmp->state);
	return 0;
}

static int g729tolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	g729_decoder_pvt *tmp = pvt->pvt;
	int frameSize;
	int x;
	for (x = 0; x < f->datalen; x+= frameSize)
	{
		if (f->datalen-x >= 10) 
		{
			frameSize = 10; //Regular
		} else
		if (f->datalen-x == 2)
		{
			frameSize = 2; //VAD
		} else
		{
			ast_log(LOG_WARNING, "Invalid data (%d bytes at the end)\n", f->datalen-x);
			return -1;
		}

		if (pvt->samples+G729_SAMPLES > BUFFER_SAMPLES)
		{
			ast_log(LOG_WARNING, "Out of buffer space\n");
			return -1;
		}

		int16_t *dst = pvt->OUTBUF_SLIN + pvt->samples;
		g729a_decoder(&tmp->state, (unsigned char*)f->FRAME_DATA + x, dst, FrameSize);
		pvt->samples += G729_SAMPLES;
		pvt->datalen += 2 * G729_SAMPLES;
	}
	return 0;
}

//#include "g729_slin_ex.h"
unsigned char g729_slin_ex[50] =
	{ 0x8F, 0x65, 0x7D, 0x4E, 0x3E, 0x0A, 0xF2, 0xBA, 0xF1, 0x5E,
	  0x0F, 0xB6, 0x73, 0xB9, 0x33, 0x62, 0xB8, 0x2B, 0x1A, 0x57,
	  0x31, 0xF8, 0x37, 0xC4, 0xE0, 0x5A, 0xF0, 0x8C, 0x8D, 0x50,
	  0xA3, 0xB7, 0xC1, 0x9D, 0xB5, 0x60, 0x70, 0x36, 0x39, 0x1E,
	  0x09, 0x4C, 0x2F, 0xC2, 0xA7, 0xB7, 0xEE, 0x37, 0x18, 0x52 };

static struct ast_frame *g729tolin_sample(void)
{
        static struct ast_frame f = {
                .frametype = AST_FRAME_VOICE,
                .datalen = 50, //sizeof(g729_slin_ex),
                .samples = 400, //ARRAY_LEN(g729_slin_ex) * 8,
                .mallocd = 0,
                .offset = 0,
                .src = __PRETTY_FUNCTION__,
                .FRAME_DATA = g729_slin_ex,
        };

        f.subclass.format = ast_format_g729;

        return &f;
}

static struct ast_translator g729tolin = {
	.name = "g729tolin",
	.src_codec = {
		.name = "g729",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.dst_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.format = "slin",
	.newpvt = g729tolin_new,
	.framein = g729tolin_framein,
	.sample = g729tolin_sample,
	.desc_size = sizeof(struct g729_decoder_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

//------------ ENCODER

struct g729_encoder_pvt {
	struct encoder_state state;
	int16_t buf[BUFFER_SAMPLES];
};


static int lintog729_new(struct ast_trans_pvt *pvt)
{
	g729a_encoder_init(&pvt->pvt->state);
	return 0;
}

static int lintog729_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	g729_encoder_pvt *tmp = pvt->pvt;
	if (pvt->samples + f->samples > BUFFER_SAMPLES)
	{
		ast_log(LOG_WARNING, "Out of buffer space\n");
		return -1;
	}
	memcpy(tmp->buf+pvt->samples, f->FRAME_DATA, 2 * f->samples);
	pvt->samples += f->samples;
	return 0;
}

static struct ast_frame *lintog729_frameout(struct ast_trans_pvt *pvt)
{
	g729_encoder_pvt *tmp = pvt->pvt;
	if (pvt->samples < G729_SAMPLES)
		return NULL;
	int datalen = 0;
	int samples = 0;
	while (pvt->samples >= G729_SAMPLES)
	{
		int frameSize;
		g729a_encoder(&tmp->state, tmp->buf+samples, (unsigned char *)(pvt->OUTBUF_G729)+datalen, &frameSize);
		datalen += G729_FRAME_LEN; //frameSize
		samples += G729_SAMPLES;
		pvt->samples -= G729_SAMPLES;

	}
	if (pvt->samples)
		memmove(tmp->buf, tmp->buf+samples, pvt->samples * 2);
	return ast_trans_frameout(pvt, datalen, samples);
}

//#include "slin_g729_ex.h"
static signed short slin_g729_ex[240] = {
0x0873, 0x06d9, 0x038c, 0x0588, 0x0409, 0x033d, 0x0311, 0xff6c, 0xfeef, 0xfd3e,
0xfdff, 0xff7a, 0xff6d, 0xffec, 0xff36, 0xfd62, 0xfda7, 0xfc6c, 0xfe67, 0xffe1,
0x003d, 0x01cc, 0x0065, 0x002a, 0xff83, 0xfed9, 0xffba, 0xfece, 0xff42, 0xff16,
0xfe85, 0xff31, 0xff02, 0xfdff, 0xfe32, 0xfe3f, 0xfed5, 0xff65, 0xffd4, 0x005b,
0xff88, 0xff01, 0xfebd, 0xfe95, 0xff46, 0xffe1, 0x00e2, 0x0165, 0x017e, 0x01c9,
0x0182, 0x0146, 0x00f9, 0x00ab, 0x006f, 0xffe8, 0xffd8, 0xffc4, 0xffb2, 0xfff9,
0xfffe, 0x0023, 0x0018, 0x000b, 0x001a, 0xfff7, 0x0014, 0x000b, 0x0004, 0x000b,
0xfff1, 0xff4f, 0xff3f, 0xff42, 0xff5e, 0xffd4, 0x0014, 0x0067, 0x0051, 0x003b,
0x0034, 0xfff9, 0x000d, 0xff54, 0xff54, 0xff52, 0xff3f, 0xffcc, 0xffe6, 0x00fc,
0x00fa, 0x00e4, 0x00f3, 0x0021, 0x0011, 0xffa1, 0xffab, 0xffdb, 0xffa5, 0x0009,
0xffd2, 0xffe6, 0x0007, 0x0096, 0x00e4, 0x00bf, 0x00ce, 0x0048, 0xffe8, 0xffab,
0xff8f, 0xffc3, 0xffc1, 0xfffc, 0x0002, 0xfff1, 0x000b, 0x00a7, 0x00c5, 0x00cc,
0x015e, 0x00e4, 0x0094, 0x0029, 0xffc7, 0xffc3, 0xff86, 0xffe4, 0xffe6, 0xffec,
0x000f, 0xffe3, 0x0028, 0x004b, 0xffaf, 0xffcb, 0xfedd, 0xfef8, 0xfe83, 0xfeba,
0xff94, 0xff94, 0xffbe, 0xffa8, 0xff0d, 0xff32, 0xff58, 0x0021, 0x0087, 0x00be,
0x0115, 0x007e, 0x0052, 0xfff0, 0xffc9, 0xffe8, 0xffc4, 0x0014, 0xfff0, 0xfff5,
0xfffe, 0xffda, 0x000b, 0x0010, 0x006f, 0x006f, 0x0052, 0x0045, 0xffee, 0xffea,
0xffcb, 0xffdf, 0xfffc, 0xfff0, 0x0012, 0xfff7, 0xfffe, 0x0018, 0x0050, 0x0066,
0x0047, 0x0028, 0xfff7, 0xffe8, 0xffec, 0x0007, 0x001d, 0x0016, 0x00c4, 0x0093,
0x007d, 0x0052, 0x00a5, 0x0091, 0x003c, 0x0041, 0xffd1, 0xffda, 0xffc6, 0xfff0,
0x001d, 0xfffe, 0x0024, 0xffee, 0xfff3, 0xfff0, 0xffea, 0x0012, 0xfff3, 0xfff7,
0xffda, 0xffca, 0xffda, 0xffdf, 0xfff3, 0xfff7, 0xff54, 0xff7c, 0xff8c, 0xffb9,
0x0012, 0x0012, 0x004c, 0x0007, 0xff50, 0xff66, 0xff54, 0xffa9, 0xffdc, 0xfff9,
0x0038, 0xfff9, 0x00d2, 0x0096, 0x008a, 0x0079, 0xfff5, 0x0019, 0xffad, 0xfffc };


static struct ast_frame *lintog729_sample(void)
{
        static struct ast_frame f = {
                .frametype = AST_FRAME_VOICE,
                .datalen = 480, //sizeof(slin_g729_ex),
                .samples = 240, //ARRAY_LEN(slin_g729_ex),
                .mallocd = 0,
                .offset = 0,
                .src = __PRETTY_FUNCTION__,
                .FRAME_DATA = slin_g729_ex,
        };

        f.subclass.format = ast_format_slin;

        return &f;
}


static struct ast_translator lintog729 = {
	.name = "lintog729",
	.src_codec = {
		.name = "slin",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.dst_codec = {
		.name = "g729",
		.type = AST_MEDIA_TYPE_AUDIO,
		.sample_rate = 8000,
	},
	.format = "g729",
	.newpvt = lintog729_new,
	.framein = lintog729_framein,
	.frameout = lintog729_frameout,
	.sample = lintog729_sample,
	.desc_size = sizeof (struct g729_encoder_pvt),
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
};

static int unload_module(void)
{
        int res;

        res = ast_unregister_translator(&lintog729);
        res |= ast_unregister_translator(&g729tolin);

        return res;
}

static int load_module(void)
{
        int res = 0;

        res = ast_register_translator(&g729tolin);
        res |= ast_register_translator(&lintog729);

        if (res) {
                unload_module();
                return AST_MODULE_LOAD_DECLINE;
        }

        return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "G.729 Annex A (floating point) Codec",
        .support_level = AST_MODULE_SUPPORT_CORE,
        .load = load_module,
        .unload = unload_module,
);
