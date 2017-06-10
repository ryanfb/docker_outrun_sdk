FROM ryanfb/m68k_gcc:latest

ENV OUTRUN_SDK /opt/outrun
RUN mkdir $OUTRUN_SDK
WORKDIR $OUTRUN_SDK
ADD . $OUTRUN_SDK

# Build Outrun SDK
RUN cd sdk && pwd && ls && bash ./build_sdk.sh && ls -sh lib

ENV BUILD_DIR $OUTRUN_SDK/samples/audio

# Build individual sample
RUN cd $BUILD_DIR && bash ./make.sh

# Use wine to run splitbin.exe to produce final roms
FROM suchja/wine:dev
USER root

ENV OUTRUN_SDK /opt/outrun
ENV BUILD_DIR $OUTRUN_SDK/samples/audio

COPY --from=0 $OUTRUN_SDK $OUTRUN_SDK
RUN chown -R xclient $OUTRUN_SDK
USER xclient
RUN wine wineboot --init
RUN wine $OUTRUN_SDK/bin/splitbin.exe

ENV OUTPUT_PATH $BUILD_DIR/output
RUN wine $OUTRUN_SDK/bin/splitbin.exe $OUTPUT_PATH/maincpu_rom.bin 65536 2 $OUTPUT_PATH/epr-10380b.133 $OUTPUT_PATH/epr-10382b.118 $OUTPUT_PATH/epr-10381b.132 $OUTPUT_PATH/epr-10383b.117
RUN wine $OUTRUN_SDK/bin/splitbin.exe $OUTPUT_PATH/subcpu_rom.bin 65536 2 $OUTPUT_PATH/epr-10327a.76 $OUTPUT_PATH/epr-10329a.58 $OUTPUT_PATH/epr-10328a.75 $OUTPUT_PATH/epr-10330a.57
RUN ls -sh $OUTPUT_PATH

USER root
RUN mkdir -p /roms
RUN cp -v $OUTPUT_PATH/epr-* /roms
