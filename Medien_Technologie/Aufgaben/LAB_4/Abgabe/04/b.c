#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x264.h>


#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

/*
	Neu: FFmpeg MP4 MUXER SETUP, AV_Packete, MP4 Trailer
	Ausführung: make->z.B. ./MTM04b.exe foreman.yuv 352 288 foreman.mp4
	video kann mit ffplay foreman.mp4 abgespielt werden.
	
	Ausgangsbasis File a
	
	Searched for H.264-kodierten into mp4 plain c code, (Stack Overflow/Other Forums and Microsoft/Github didn't yield much)
	Honorable Mentions:
	https://pure.tue.nl/ws/files/68505334/hernanzimplementation2013.pdf [p.26,p.29-31]
	https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html

	
	the most useful was the ai-generated response giving a general program, used that for reference.
	"int main(int argc, char *argv[]) {
    const char *in_filename, *out_filename;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, stream_index = 0;
	...Error Handling
	// 2. Ausgabedatei erstellen
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    ...Error H
    // 3. Videostream aus der Eingabe kopieren
    AVStream *in_stream = ifmt_ctx->streams[0];
    AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
    ...
    // Codec-Parameter kopieren
    avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    out_stream->codecpar->codec_tag = 0; // Standard-Tag für Container setzen
    ...Header Schreiben
    while(1){
    ...
    // Zeiten umrechnen
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    // Paket schreiben
    if ((ret = av_interleaved_write_frame(ofmt_ctx, &pkt)) < 0) {
	...
	}
	av_write_trailer(ofmt_ctx);
	... cleanup
	}
	
	ffmpeg (!!only minimum for 10MIB Limit!! ./configure):
	./configure --prefix=/home/stud/Desktop/Laboruebungen/LAB_4/Abgabe/libs \
            --enable-static \
            --disable-shared \
            --enable-gpl \
            --enable-libx264 \
            --disable-everything \
            --enable-encoder=libx264 \
            --enable-decoder=h264 \
            --enable-muxer=mp4 \
            --enable-muxer=mov \
            --enable-protocol=file \
*/

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Bitte gib: %s <Pfad> <Breite> <Hoehe> <Ausgabe-Pfad> an\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    //konvertiert einen ASCII-String in eine Ganzzahl
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    char *out_filename = argv[4];

    if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
        fprintf(stderr, "Fehler: Ungueltige Dimensionen (muessen gerade und positiv sein).\n");
        return EXIT_FAILURE;
    }
	//read binary
    FILE *file_in = fopen(filename, "rb");
    if (!file_in) {
        perror("Fehler beim Oeffnen der Datei");
        return EXIT_FAILURE;
    }
    
    // Speicherberechnung für ein Frame im 4:2:0 Format
    //Y full, U 1/4, V 1/4
    size_t y_size = (size_t)width * height;
    size_t uv_size = (size_t)(width / 2) * (height / 2);//Viertel
    size_t frame_size = y_size + 2 * uv_size;//z.B. 352*288*1,5

    unsigned char *buffer = (unsigned char *)malloc(frame_size);
    if (!buffer) {
        fprintf(stderr, "Speicherfehler.\n");
        fclose(file_in);
        return EXIT_FAILURE;
    }
    
    // --- x264 Initialisierung ---
    x264_param_t param;
    x264_picture_t pic_in;
    x264_picture_t pic_out;
    x264_t *encoder;

    // Standard-Parameter setzen (Preset: medium, Tune: standard/none)
    if (x264_param_default_preset(&param, "medium", NULL) < 0) {
        fprintf(stderr, "Fehler beim Setzen des Presets.\n");
        fclose(file_in);
        return EXIT_FAILURE;
    }

    // Video-Eigenschaften konfigurieren
    param.i_csp = X264_CSP_I420; // IYUV / YUV 4:2:0 planar
    param.i_width = width;
    param.i_height = height;
    param.b_vfr_input = 0;       // Konstante Framerate
    param.i_fps_num = 25;        // Standard: 25 Bilder pro Sekunde
    param.i_fps_den = 1;

    // Basis-Profil erzwingen (erhoeht Kompatibilitaet)
    if (x264_param_apply_profile(&param, "baseline") < 0) {
        fprintf(stderr, "Fehler beim Anwenden des Profils.\n");
        fclose(file_in);
        return EXIT_FAILURE;
    }

    // Zuweisung des Eingangsbild-Puffers gemäss Farbraum
    if (x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height) < 0) {
        fprintf(stderr, "Fehler bei der Speicherallokation fuer das Bild.\n");
        fclose(file_in);
        return EXIT_FAILURE;
    }

    // Encoder oeffnen
    encoder = x264_encoder_open(&param);
    if (!encoder) {
        fprintf(stderr, "Fehler beim Oeffnen des Encoders.\n");
        x264_picture_clean(&pic_in);
        fclose(file_in);
        return EXIT_FAILURE;
    }
    
    // NEU: FFmpeg MP4 MUXER SETUP
    AVFormatContext *fmt_ctx = NULL;
    AVStream *video_stream = NULL;
    AVPacket *pkt = av_packet_alloc();

    // Output Context fuer MP4 erstellen
    if (avformat_alloc_output_context2(&fmt_ctx, NULL, "mp4", out_filename) < 0) {
        fprintf(stderr, "Fehler beim Erstellen des Ausgabe-Contexts.\n");
        return EXIT_FAILURE;
    }

    // Neuen Video-Stream im MP4-Container anlegen
    video_stream = avformat_new_stream(fmt_ctx, NULL);
    if (!video_stream) {
        fprintf(stderr, "Fehler beim Erstellen des Videostreams.\n");
        return EXIT_FAILURE;
    }

    // Parameter fuer den Stream definieren
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    video_stream->codecpar->width = width;
    video_stream->codecpar->height = height;
    video_stream->time_base = (AVRational){1, 25}; //25 FPS

    // Datei oeffnen
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Konnte Ausgabe-MP4 nicht oeffnen.\n");
            return EXIT_FAILURE;
        }
    }

    // MP4 Header schreiben
    if (avformat_write_header(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Fehler beim Schreiben des MP4 Headers.\n");
        return EXIT_FAILURE;
    }


    int frame_count = 0;

    x264_nal_t *nals;
    int i_nals;
    
    // Liest Frames nacheinander ein, solange ganze frames gelesen, kodiert und muxing
    
    while (fread(buffer, 1, frame_size, file_in) == frame_size) {
        printf("Frame %d eingelesen:\n\n", frame_count);

        int y = 2;
        int x = 2;
        unsigned char y_val = buffer[y * width + x];
        unsigned char u_val = buffer[y_size + (y / 2) * (width / 2) + (x / 2)];
        unsigned char v_val = buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)];
        printf("Pixel an (x=%d, y=%d): Y=%d, U=%d, V=%d\n", x, y, y_val, u_val, v_val);

        if(frame_count<10){
            printf("Y-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y * width + x], (size_t)(frame_count * frame_size) + (y * width + x));
            printf("U-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + (y / 2 * width / 2 + x / 2));
            printf("V-Wert [index %d,%d]: %u (Offset im File: %zu)\n", x, y, buffer[y_size + uv_size + (y / 2) * (width / 2) + (x / 2)], (size_t)(frame_count * frame_size) + y_size + uv_size + (y / 2 * width / 2 + x / 2));
        }

        // Frame an x264 übergeben und kodieren
        // Kopiert die Daten aus deinem 'buffer' in die x264-Struktur
        memcpy(pic_in.img.plane[0], buffer, y_size);
        memcpy(pic_in.img.plane[1], buffer + y_size, uv_size);
        memcpy(pic_in.img.plane[2], buffer + y_size + uv_size, uv_size);
        
        pic_in.i_pts = frame_count;

        int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
        
        if (frame_size_out > 0) {
			// Alle NAL-Einheiten dieses Frames in ein FFmpeg AVPacket packen
            av_new_packet(pkt, frame_size_out);
            // Da x264 alle NALs fortlaufend im Speicher ablegt, koennen wir sie am Stueck kopieren
            memcpy(pkt->data, nals[0].p_payload, frame_size_out);
            
            // Zeitstempel fuer FFmpeg setzen
            pkt->pts = pic_out.i_pts;
            pkt->dts = pic_out.i_dts;
            pkt->stream_index = video_stream->index;
            
            // Paket direkt an libavformat uebergeben (In die MP4 muxen)
            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
            
            //wird jetzt auch ueber ffmpeg geschrieben
            //fwrite(nals[0].p_payload, 1, frame_size_out, file_out);
        }
        frame_count++;
    }

    // Delayed Frames leeren (Flush)
    while (x264_encoder_delayed_frames(encoder)) {
		int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, NULL, &pic_out);
        
        if (frame_size_out > 0) {
            av_new_packet(pkt, frame_size_out);
            memcpy(pkt->data, nals[0].p_payload, frame_size_out);
            
            pkt->pts = pic_out.i_pts;
            pkt->dts = pic_out.i_dts;
            pkt->stream_index = video_stream->index;

            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
        }
    }
    // MP4 Trailer (Datei-Ende) schreiben
    av_write_trailer(fmt_ctx);
    
    
    x264_encoder_close(encoder);
    x264_picture_clean(&pic_in);
    
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    
    avformat_free_context(fmt_ctx);
    av_packet_free(&pkt);
    
    free(buffer);
    fclose(file_in);
        
    return EXIT_SUCCESS;
}
