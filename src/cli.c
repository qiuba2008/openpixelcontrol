/* Copyright 2016 Ka-Ping Yee

Licensed under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy
of the License at: http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.  See the License for the
specific language governing permissions and limitations under the License. */

#include "cli.h"
#include "spi.h"

void get_speed_and_port(u32* speed, u16* port, int argc, char** argv) {
  if (argc > 1 && speed) {
    *speed = strtol(argv[1], 0, 10)*1000000;
  }
  if (argc > 2 && port) {
    *port = atoi(argv[2]);
  }
}

static int spi_fd = -1;
static u8* put_pixels_buffer;
static put_pixels_func* put_pixels;

void opc_serve_handler(u8 address, u16 count, pixel* pixels) {
  fprintf(stderr, "%d ", count);
  fflush(stderr);
  put_pixels(spi_fd, put_pixels_buffer, count, pixels);
}

int opc_serve_main(char* spi_device_path, u32 spi_speed_hz, u16 port,
                    put_pixels_func* put, u8* buffer) {
  pixel diagnostic_pixel;
  time_t t;
  u16 inactivity_ms = 0;

  spi_fd = init_spidev(spi_device_path, spi_speed_hz);
  if (spi_fd < 0) {
    fprintf(stderr, "Could not open SPI device at %s\n", spi_device_path);
    return 1;
  }
  opc_source s = opc_new_source(port);
  if (s < 0) {
    fprintf(stderr, "Could not create OPC source\n");
    return 1;
  }
  fprintf(stderr, "SPI speed: %.2f MHz, ready...\n", spi_speed_hz*1e-6);
  put_pixels = put;
  put_pixels_buffer = buffer;
  while (inactivity_ms < INACTIVITY_TIMEOUT_MS) {
      if (opc_receive(s, opc_serve_handler, DIAGNOSTIC_TIMEOUT_MS)) {
          inactivity_ms = 0;
      } else {
          inactivity_ms += DIAGNOSTIC_TIMEOUT_MS;
          t = time(NULL);
          diagnostic_pixel.r = (t % 3 == 0) ? 64 : 0;
          diagnostic_pixel.g = (t % 3 == 1) ? 64 : 0;
          diagnostic_pixel.b = (t % 3 == 2) ? 64 : 0;
          put_pixels(spi_fd, buffer, 1, &diagnostic_pixel);
      }
  }
  fprintf(stderr, "Exiting after %d ms of inactivity\n",
          INACTIVITY_TIMEOUT_MS);
  return 0;
}
