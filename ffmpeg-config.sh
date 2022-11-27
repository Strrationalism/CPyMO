#!/bin/bash

if [ "--audio" == "$1" ]; then
    DEC=""
    PAR=""
    DEM=""

    # mp3
    DEC+="mp3,mp3_at,mp3adu,mp3adufloat,mp3float,mp3on4,mp3on4float,"
    PAR+="mpegaudio,"
    DEM+="mp3,"
    
    # ogg
    DEC+="vorbis,"
    PAR+="vorbis,"
    DEM+="ogg,"

    # amr
    DEC+="amr_nb_at,amrnb,amrwb,"
    PAR+="amr,"
    DEM+="amr,amrnb,armwb,"

    # aac
    DEC+="aac,aac_at,aac_fixed,aac_latm,"
    PAR+="aac,aac_latm,"
    DEM+="aac,"

    #flac
    DEC+="flac,"
    PAR+="flac,"
    DEM+="flac,"


    # wav
    PCMS="pcm_u8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,"
    PCMS+="pcm_s8,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,"
    PCMS+="pcm_f32be,pcm_f32le,pcm_f64le,pcm_f64be,"
    PCMS+="pcm_s16be_planar,pcm_s16le_planar,pcm_s24le_planar,pcm_s32le_planar,pcm_s8_planar,"
    DEM+=$PCMS
    DEM+="wav,"
    DEC+=$PCMS

    # output
    DEM=`echo ${DEM%?}`
    PAR=`echo ${PAR%?}`
    DEC=`echo ${DEC%?}`
    echo "--enable-decoder=$DEC --enable-parser=$PAR --enable-demuxer=$DEM"

#elif [ "--video" -n $1 ]; then

else
    echo "./ffmpeg-config.sh --audio"
    echo "./ffmpeg-config.sh --video"
fi