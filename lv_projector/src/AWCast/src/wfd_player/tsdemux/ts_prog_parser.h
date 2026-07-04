
#ifndef TS_PROG_PARSER_H
#define TS_PROG_PARSER_H
#include <ts_types.h>

typedef struct TPPctxS TPPhandlerT;

struct ProgramInfoS
{
    uint32_t index;
    uint32_t valid;
	uint32_t pmt_pid;
    uint32_t video_pid;
    uint32_t video_codec_type;
    uint32_t audio_codec_type;
    uint32_t audio_num;
    uint32_t audio_pid[PROGRAM_AUDIO_MAX];
	char name[PROGRAM_NAME_LENGTH];
};

#ifdef __cplusplus
extern "C"
{
#endif

TPPhandlerT *TPP_Instance();

/* 0: OK; -1: somthing err; 1/2/3/4/5/...:program parse finish. */
int TPP_Fill(TPPhandlerT *tpp, TSBufferT *buf);

int TPP_GetProgram(TPPhandlerT *tpp, struct ProgramInfoS *pi);

int TPP_Destroy(TPPhandlerT *tpp);

#ifdef __cplusplus
}
#endif

#endif
