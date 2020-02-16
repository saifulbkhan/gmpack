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

#include "gmpackpacker.h"
#include "testutils.h"

typedef struct {
  GmpackPacker *packer;
  GList        *samples;
} PackerFixture;

static void
packer_fixture_set_up (PackerFixture *fixture,
                       gconstpointer  user_data)
{
  fixture->packer = gmpack_packer_new ();
  if (user_data != NULL) {
    SampleListGenerator create_samples = user_data;
    fixture->samples = create_samples ();
  }
}

static void
packer_fixture_tear_down (PackerFixture *fixture,
                          gconstpointer  user_data)
{
  g_list_free_full (fixture->samples, sample_free);
  g_clear_object (&fixture->packer);
}

static void
test_packer_pack_variant (PackerFixture *fixture,
                          gconstpointer  user_data)
{
  GList *l = NULL;

  for (l = fixture->samples; l != NULL; l = l->next) {
    gchar* packed = NULL;
    gsize packed_length = 0;
    g_autoptr (GError) error = NULL;
    g_autoptr (GBytes) bytes = NULL;
    GBytes *rep_bytes = NULL;
    Sample *test_sample = l->data;

    packed_length = gmpack_packer_pack_variant (fixture->packer,
                                                test_sample->variant,
                                                &packed,
                                                &error);
    g_assert_no_error (error);

    bytes = g_bytes_new_take (packed, packed_length);
    rep_bytes = shortest_rep (test_sample->variant, fixture->samples);
    g_assert_nonnull (rep_bytes);

    g_assert_true (g_bytes_equal (bytes, rep_bytes));
  }
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add ("/gmpack/packer/pack-variant-nil",
              PackerFixture,
              nil_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-bool",
              PackerFixture,
              bool_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-binary",
              PackerFixture,
              binary_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-number-positive",
              PackerFixture,
              number_positive_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-number-negative",
              PackerFixture,
              number_negative_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-number-float",
              PackerFixture,
              number_float_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-number-bignum",
              PackerFixture,
              number_bignum_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-ascii",
              PackerFixture,
              string_ascii_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-utf8",
              PackerFixture,
              string_utf8_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-emoji",
              PackerFixture,
              string_emoji_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-array",
              PackerFixture,
              array_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-map",
              PackerFixture,
              map_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  g_test_add ("/gmpack/packer/pack-variant-nested",
              PackerFixture,
              nested_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);
  /* TODO: add tests for timestamp (ext) type */
  g_test_add ("/gmpack/packer/pack-variant-ext",
              PackerFixture,
              ext_samples,
              packer_fixture_set_up,
              test_packer_pack_variant,
              packer_fixture_tear_down);

  return g_test_run ();
}
