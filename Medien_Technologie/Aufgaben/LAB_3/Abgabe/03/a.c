#include <stdio.h>
#include <libavutil/avutil.h>

/* * FFmpeg-KONFIGURATION (./configure)
 * --prefix=../../Laboruebungen/LAB_3/Abgabe/libs
 * lokaler Installationspfad
 *
 * --disable-everything
 * Deaktiviert standardmäßig ALLE Codecs, Demuxer, Muxer und Protokolle.
 *
 * --disable-autodetect
 * Verhindert, dass FFmpeg automatisch nach externen Systembibliotheken sucht.
 *
 * --enable-demuxer=mp3
 * Erlaubt es, das MP3-Containerformat zu lesen und daraus Audio zu extrahieren.
 *
 * --enable-decoder=mp3
 * Aktiviert den MP3-Dekodierer, um die komprimierten 
 * Audiodaten der Eingabedatei in rohe Samples umzuwandeln.
 *
 * --enable-encoder=aac
 * Aktiviert den AAC-Enkodierer für die Ziel-Komprimierung.
 *
 * --enable-encoder=opus
 * Aktiviert den Opus-Enkodierer als moegliche alternative zu AAC. 
 *
 * --enable-muxer=mp4
 * Erlaubt es, die enkodierten Audiodaten in das MP4-Containerformat 
 * zusammenzufügen.
 *
 * --enable-protocol=file
 * Ermöglicht der Bibliothek zugriff auf die 
 * lokale Festplatte, um Dateien zu lesen und zu schreiben.
 *
 * --disable-programs
 * Deaktiviert den Bau der fertigen Kommandozeilen-Tools (ffmpeg, ffplay, ffprobe).
 *
 * --disable-doc / --disable-network / --disable-avdevice / --disable-swscale / --disable-avfilter
 * Schaltet alle unbenötigten Zusatzmodule ab (Dokumentation, Netzwerkfunktionen, 
 * Hardware-Gerätezugriffe, Bildskalierung und Videofilter)
 * 
 * Information Source mostly: https://gist.github.com/omegdadi/6904512c0a948225c81114b1c5acb875
 */
 
int main(void) {
	//"Return an informative version string.", gets the actual release version
    printf("FFmpeg libavutil Version: %s\n", av_version_info());
    //Output: FFmpeg libavutil Version: 8.0.1
    return 0;
}
