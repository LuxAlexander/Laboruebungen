#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>

#define OUTPUT_BIT_RATE 128000  // Gewuenschte Ziel-Bitrate für das AAC-Audio
//es wurde bei Aufgabe 1.a fuer Audio Jahzzar_-_01_-_Out_of_School.mp3, eine minimale rate von 28kbit/s ermittelt.
#define OUTPUT_CHANNELS 2      // Stereo-Ausgabe

/*
 * Encoder:https://ffmpeg.org/doxygen/8.0/encode_audio_8c-example.html
 * Decoder:https://ffmpeg.org/doxygen/8.0/decode_audio_8c-example.html
 * Transcoder-Vorlage:https://ffmpeg.org/doxygen/8.0/transcode_aac_8c-example.html
 * Das Programm ist eine minimale Version von transcode_aac_8c-example.
 * Aenderungen zu b: hoehere OUTPUT BIT_RATE suche statt AAC, nach OPUS und aktiviere FF_COMPLIANCE_EXPERIMENTAL (erlaube experimental opus)
 * Ausfuehrung nach make z.b: ./TK03c.exe Jahzzar_-_01_-_Out_of_School.mp3 out_opus.mp4
*/


static int64_t pts = 0;        // Zeitstempel-Zaehler für die Ausgabe-Frames

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.mp3> <output.mp4>\n", argv[0]);
        return 1;
    }

    // INPUT DATEI oeffnen & DECODER init
    AVFormatContext *ifmt_ctx = NULL;
    // lese die Eingangsdatei und liest die grundlegenden Header-Informationen
    avformat_open_input(&ifmt_ctx, argv[1], NULL, NULL);
    // Analysiert den Datenstrom detailliert, um Codec-Informationen zu finden
    avformat_find_stream_info(ifmt_ctx, NULL);

    //erste Stream (Index 0) ist die Audiospur
    AVStream *istream = ifmt_ctx->streams[0];
    // Findet den passenden MP3-Decoder anhand der ID aus dem Eingangsstream
    const AVCodec *idec = avcodec_find_decoder(istream->codecpar->codec_id);
    // Reserviert Speicher für den Kontext (Zustand) des Decoders
    AVCodecContext *idec_ctx = avcodec_alloc_context3(idec);
    // Kopiert die Codec-Parameter (Abtastrate, Kanaele) aus der Datei in den Decoder-Kontext
    avcodec_parameters_to_context(idec_ctx, istream->codecpar);
    // Startet den Decoder, sodass er bereit zum Dekodieren ist
    avcodec_open2(idec_ctx, idec, NULL);
    // Uebergibt die Zeitbasis der Datei an den Decoder, um korrekte Paket-Timings zu haben
    idec_ctx->pkt_timebase = istream->time_base;


    // OUTPUT DATEI oeffnen & ENCODER init
    AVFormatContext *ofmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    // oeffne den Ausgabe-Dateistream im Schreibmodus
    avio_open(&avio_ctx, argv[2], AVIO_FLAG_WRITE);
    // Erstellt den Hauptkontext für das Ausgabe-Containerformat
    ofmt_ctx = avformat_alloc_context();
    ofmt_ctx->pb = avio_ctx; // Verknuepft den Dateistream mit dem Formatkontext
    // Ermittelt das passende Containerformat anhand der Dateiendung (z.B. mp4)
    ofmt_ctx->oformat = av_guess_format(NULL, argv[2], NULL);
    ofmt_ctx->url = av_strdup(argv[2]);

	//Suche nach OPUS, statt AAC
    // Findet den eingebauten Encoder von FFmpeg
    const AVCodec *oenc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    // Erstellt eine neue Spur (Stream) innerhalb der Ausgabedatei
    AVStream *ostream = avformat_new_stream(ofmt_ctx, NULL);
    // Reserviert Speicher für den Kontext des Encoders
    AVCodecContext *oenc_ctx = avcodec_alloc_context3(oenc);

    // Setzt das Kanal-Layout standardmaessig auf Stereo (2 Kanäle)
    av_channel_layout_default(&oenc_ctx->ch_layout, OUTPUT_CHANNELS);
    // OPUS minimum 48000
    oenc_ctx->sample_rate = 48000;
    // Waehlt das vom AAC-Encoder Sampleformat
    oenc_ctx->sample_fmt = oenc->sample_fmts[0];
    oenc_ctx->bit_rate = OUTPUT_BIT_RATE;
    // Setzt die Zeitbasis des Ausgabestreams passend zur Sample-Frequenz (1 / Samplerate)
    ostream->time_base = (AVRational){1, oenc_ctx->sample_rate};

	// OPUS experimentell eingestufft, deshalb (termianl -strict -2)
    oenc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    
    // MP4 benoetigt oft globale Header-Metadaten statt lokaler Paket-Header
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        oenc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Aktiviert und oeffnet den Encoder mit den Einstellungen
    avcodec_open2(oenc_ctx, oenc, NULL);
    // Uebermittelt die fertig konfigurierten Encoder-Parameter an die Ausgabespur
    avcodec_parameters_from_context(ostream->codecpar, oenc_ctx);
    // Schreibt den MP4-Dateikopf (Header) in die Ausgabedatei
    avformat_write_header(ofmt_ctx, NULL);


    // RESAMPLER & FIFO init
    SwrContext *swr_ctx = NULL;
    // Konfiguriert den Resampler, um mögliche Differenzen in Kanaelen/Sampleformaten auszugleichen
    swr_alloc_set_opts2(&swr_ctx, &oenc_ctx->ch_layout, oenc_ctx->sample_fmt, oenc_ctx->sample_rate,
                                  &idec_ctx->ch_layout, idec_ctx->sample_fmt, idec_ctx->sample_rate, 0, NULL);
    // Aktiviert den Resampler
    swr_init(swr_ctx);

    // Erstellt einen FIFO-Zwischenspeicher. Noetig, da MP3 andere Paketgrößen liefert (1152), als AAC braucht (1024)
    AVAudioFifo *fifo = av_audio_fifo_alloc(oenc_ctx->sample_fmt, oenc_ctx->ch_layout.nb_channels, 1);

    // TRANSKODIERUNGS-Schleife
    // Reserviert Speicher für Roh-Pakete (komprimiert) und Frames (unkomprimiert)
    AVPacket *ipack = av_packet_alloc();
    AVFrame *iframe = av_frame_alloc();
    int finished = 0;

    while (1) {
        // Solange im FIFO nicht genug Samples für einen vollen AAC-Frame sind und die Datei nicht zu Ende ist:
        while (av_audio_fifo_size(fifo) < oenc_ctx->frame_size && !finished) {
            // Liest das naechste komprimierte MP3-Paket aus der Eingangsdatei
            if (av_read_frame(ifmt_ctx, ipack) < 0) {
                finished = 1; // Dateiende erreicht
                break;
            }
            // Schickt das komprimierte Paket an den Decoder
            if (avcodec_send_packet(idec_ctx, ipack) == 0) {
                // Holt alle fertig dekodierten, unkomprimierten Audio-Frames ab
                while (avcodec_receive_frame(idec_ctx, iframe) == 0) {
                    uint8_t **converted_samples = NULL;
                    // Reserviert temporär Speicher für das konvertierte Audio-Format
                    av_samples_alloc_array_and_samples(&converted_samples, NULL, oenc_ctx->ch_layout.nb_channels, iframe->nb_samples, oenc_ctx->sample_fmt, 0);
                    // Konvertiert die Audiodaten ins passende Format für den AAC-Encoder
                    swr_convert(swr_ctx, converted_samples, iframe->nb_samples, (const uint8_t **)iframe->extended_data, iframe->nb_samples);
                    // Vergroessert den FIFO-Puffer dynamisch, um die neuen Samples aufzunehmen
                    av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + iframe->nb_samples);
                    // Schreibt die konvertierten Audiodaten in den FIFO-Speicher
                    av_audio_fifo_write(fifo, (void **)converted_samples, iframe->nb_samples);
                    // Gibt den temp Konvertierungsspeicher wieder frei
                    av_freep(&converted_samples[0]);
                    av_freep(&converted_samples);
                }
            }
            // Setzt das Paket zurueck, um es wiederzuverwenden
            av_packet_unref(ipack);
        }

        // Wenn die Datei zu Ende und das FIFO komplett leer ist, brechen wir die Hauptschleife ab
        if (av_audio_fifo_size(fifo) == 0 && finished) break;

        // Bestimmt die Frame-Size (normalerweise 1024 für AAC, am Dateiende eventuell weniger)
        int frame_size = FFMIN(av_audio_fifo_size(fifo), oenc_ctx->frame_size);
        AVFrame *oframe = av_frame_alloc();
        oframe->nb_samples = frame_size;
        av_channel_layout_copy(&oframe->ch_layout, &oenc_ctx->ch_layout);
        oframe->format = oenc_ctx->sample_fmt;
        oframe->sample_rate = oenc_ctx->sample_rate;
        // Reserviert den Datenpuffer innerhalb des Ausgabe-Frames
        av_frame_get_buffer(oframe, 0);

        // Holt exakt die benötigte Sample-Anzahl aus dem FIFO heraus und schreibt sie in den Ausgabe-Frame
        av_audio_fifo_read(fifo, (void **)oframe->data, frame_size);
        oframe->pts = pts; // Weist dem Frame den fortlaufenden Zeitstempel zu
        pts += frame_size; // Erhoeht den Zeitstempel um die verbrauchten Samples

        // Schickt den unkomprimierten Ausgabe-Frame an den AAC-Encoder
        if (avcodec_send_frame(oenc_ctx, oframe) == 0) {
            AVPacket *opack = av_packet_alloc();
            // Holt alle fertig komprimierten AAC-Pakete vom Encoder ab
            while (avcodec_receive_packet(oenc_ctx, opack) == 0) {
                // Schreibt das komprimierte Paket in die MP4-Ausgabedatei
                av_write_frame(ofmt_ctx, opack);
                av_packet_unref(opack);
            }
            av_packet_free(&opack);
        }
        av_frame_free(&oframe);
    }

    // ENCODER leeren (Restliche, gepufferte Frames herausholen)
    // Ein "NULL"-Frame signalisiert dem Encoder das Dateiende (Flushen)
    avcodec_send_frame(oenc_ctx, NULL);
    AVPacket *opack = av_packet_alloc();
    // Liest die allerletzten im Encoder verbliebenen Rest-Pakete aus
    while (avcodec_receive_packet(oenc_ctx, opack) == 0) {
        av_write_frame(ofmt_ctx, opack);
        av_packet_unref(opack);
    }
    av_packet_free(&opack);

    // Schreibt den Datei-Trailer (Abschluss des MP4-Containers)
    av_write_trailer(ofmt_ctx);

    // Speicher freigeben
    av_frame_free(&iframe);
    av_packet_free(&ipack);
    av_audio_fifo_free(fifo);
    swr_free(&swr_ctx);
    avcodec_free_context(&oenc_ctx);
    avcodec_free_context(&idec_ctx);
    avio_closep(&ofmt_ctx->pb); // Schließt den Dateistream der Ausgabedatei
    avformat_free_context(ofmt_ctx);
    avformat_close_input(&ifmt_ctx); // Schließt die Eingangsdatei samt Kontext

    return 0;
}
