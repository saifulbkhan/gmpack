/*
 * Copyright 2019 Saiful B. Khan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gmpackunpacker.h"
#include "testutils.h"

typedef struct {
  GmpackUnpacker *unpacker;
  GList          *samples;
} UnpackerFixture;

static void
unpacker_fixture_set_up (UnpackerFixture *fixture,
                         gconstpointer    user_data)
{
  fixture->unpacker = gmpack_unpacker_new ();
  if (user_data != NULL) {
    SampleListGenerator create_samples = user_data;
    fixture->samples = create_samples ();
  }
}

static void
unpacker_fixture_tear_down (UnpackerFixture *fixture,
                            gconstpointer    user_data)
{
  g_list_free_full (fixture->samples, sample_free);
  g_clear_object (&fixture->unpacker);
}

static void
test_unpacker_unpack_string (UnpackerFixture *fixture,
                             gconstpointer    user_data)
{
  GList *l = NULL;

  for (l = fixture->samples; l != NULL; l = l->next) {
    const gchar* string;
    gsize string_length = 0;
    g_autoptr (GError) error = NULL;
    g_autoptr (GVariant) unpacked = NULL;
    Sample *test_sample = l->data;

    string = g_bytes_get_data (test_sample->bytes, &string_length);
    unpacked = gmpack_unpacker_unpack_string (fixture->unpacker,
                                              &string,
                                              &string_length,
                                              &error);

    g_assert_no_error (error);
    g_assert_cmpvariant (unpacked, test_sample->variant);
  }
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add ("/gmpack/unpacker/unpack-string-nil",
              UnpackerFixture,
              nil_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-bool",
              UnpackerFixture,
              bool_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-binary",
              UnpackerFixture,
              binary_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-number-positive",
              UnpackerFixture,
              number_positive_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-number-negative",
              UnpackerFixture,
              number_negative_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-number-float",
              UnpackerFixture,
              number_float_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-number-bignum",
              UnpackerFixture,
              number_bignum_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-ascii",
              UnpackerFixture,
              string_ascii_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-utf8",
              UnpackerFixture,
              string_utf8_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-emoji",
              UnpackerFixture,
              string_emoji_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-array",
              UnpackerFixture,
              array_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-map",
              UnpackerFixture,
              map_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  g_test_add ("/gmpack/unpacker/unpack-string-nested",
              UnpackerFixture,
              nested_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);
  /* TODO: add tests for timestamp (ext) type */
  g_test_add ("/gmpack/unpacker/unpack-string-ext",
              UnpackerFixture,
              ext_samples,
              unpacker_fixture_set_up,
              test_unpacker_unpack_string,
              unpacker_fixture_tear_down);

  return g_test_run ();
}
