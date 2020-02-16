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

#include <glib.h>
#include <locale.h>

typedef struct {
  GVariant *variant;
  GBytes   *bytes;
} Sample;

typedef GList * (*SampleListGenerator) (void);

static void
sample_free (gpointer data)
{
  Sample* sample = data;

  g_variant_unref (sample->variant);
  g_bytes_unref (sample->bytes);
  g_slice_free (Sample, sample);
}

GBytes *
shortest_rep (GVariant* variant, GList *samples)
{
  GList *l = NULL;
  GBytes *shortest = NULL;

  for (l = samples; l != NULL; l = l->next) {
    Sample* sample = l->data;
    if (g_variant_equal (variant, sample->variant)) {
      if (shortest == NULL) {
        shortest = sample->bytes;
      } else if (g_bytes_get_size (shortest) > g_bytes_get_size (sample->bytes)) {
        // assign the shorter of the two representations
        shortest = sample->bytes;
      } else if (g_bytes_get_size (shortest) == g_bytes_get_size (sample->bytes)
                 && g_bytes_compare (shortest, sample->bytes) > 0) {
        // in case the two representations are of equal size,
        // assign the one which has smaller lexicographic value
        shortest = sample->bytes;
      }
    }
  }

  return shortest;
}

GList *
nil_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("@mv nothing");
  s1->bytes = g_bytes_new ("\xc0", 1);
  list = g_list_append (list, s1);

  return list;
}

GList *
bool_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("false");
  s1->bytes = g_bytes_new ("\xc2", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("true");
  s2->bytes = g_bytes_new ("\xc3", 1);
  list = g_list_append (list, s2);

  return list;
}

GList *
binary_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("@ay []");
  s1->bytes = g_bytes_new ("\xc4\x00", 2);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("@ay []");
  s2->bytes = g_bytes_new ("\xc5\x00\x00", 3);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("@ay []");
  s3->bytes = g_bytes_new ("\xc6\x00\x00\x00\x00", 5);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("@ay [0x01]");
  s4->bytes = g_bytes_new ("\xc4\x01\x01", 3);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("@ay [0x01]");
  s5->bytes = g_bytes_new ("\xc5\x00\x01\x01", 4);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("@ay [0x01]");
  s6->bytes = g_bytes_new ("\xc6\x00\x00\x00\x01\x01", 6);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("@ay [0x00, 0xff]");
  s7->bytes = g_bytes_new ("\xc4\x02\x00\xff", 4);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("@ay [0x00, 0xff]");
  s8->bytes = g_bytes_new ("\xc5\x00\x02\x00\xff", 5);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("@ay [0x00, 0xff]");
  s9->bytes = g_bytes_new ("\xc6\x00\x00\x00\x02\x00\xff", 7);
  list = g_list_append (list, s9);

  return list;
}

GList *
number_positive_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);
  Sample *s13 = g_slice_new0(Sample);
  Sample *s14 = g_slice_new0(Sample);
  Sample *s15 = g_slice_new0(Sample);
  Sample *s16 = g_slice_new0(Sample);
  Sample *s17 = g_slice_new0(Sample);
  Sample *s18 = g_slice_new0(Sample);
  Sample *s19 = g_slice_new0(Sample);
  Sample *s20 = g_slice_new0(Sample);
  Sample *s21 = g_slice_new0(Sample);
  Sample *s22 = g_slice_new0(Sample);
  Sample *s23 = g_slice_new0(Sample);
  Sample *s24 = g_slice_new0(Sample);
  Sample *s25 = g_slice_new0(Sample);
  Sample *s26 = g_slice_new0(Sample);
  Sample *s27 = g_slice_new0(Sample);
  Sample *s28 = g_slice_new0(Sample);
  Sample *s29 = g_slice_new0(Sample);
  Sample *s30 = g_slice_new0(Sample);
  Sample *s31 = g_slice_new0(Sample);
  Sample *s32 = g_slice_new0(Sample);
  Sample *s33 = g_slice_new0(Sample);
  Sample *s34 = g_slice_new0(Sample);
  Sample *s35 = g_slice_new0(Sample);
  Sample *s36 = g_slice_new0(Sample);
  Sample *s37 = g_slice_new0(Sample);
  Sample *s38 = g_slice_new0(Sample);
  Sample *s39 = g_slice_new0(Sample);
  Sample *s40 = g_slice_new0(Sample);
  Sample *s41 = g_slice_new0(Sample);
  Sample *s42 = g_slice_new0(Sample);
  Sample *s43 = g_slice_new0(Sample);
  Sample *s44 = g_slice_new0(Sample);
  Sample *s45 = g_slice_new0(Sample);
  Sample *s46 = g_slice_new0(Sample);
  Sample *s47 = g_slice_new0(Sample);
  Sample *s48 = g_slice_new0(Sample);
  Sample *s49 = g_slice_new0(Sample);
  Sample *s50 = g_slice_new0(Sample);
  Sample *s51 = g_slice_new0(Sample);
  Sample *s52 = g_slice_new0(Sample);
  Sample *s53 = g_slice_new0(Sample);
  Sample *s54 = g_slice_new0(Sample);
  Sample *s55 = g_slice_new0(Sample);
  Sample *s56 = g_slice_new0(Sample);
  Sample *s57 = g_slice_new0(Sample);
  Sample *s58 = g_slice_new0(Sample);
  Sample *s59 = g_slice_new0(Sample);
  Sample *s60 = g_slice_new0(Sample);
  Sample *s61 = g_slice_new0(Sample);
  Sample *s62 = g_slice_new0(Sample);
  Sample *s63 = g_slice_new0(Sample);
  Sample *s64 = g_slice_new0(Sample);
  Sample *s65 = g_slice_new0(Sample);
  Sample *s66 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("uint32 0");
  s1->bytes = g_bytes_new ("\x00", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("uint32 0");
  s2->bytes = g_bytes_new ("\xcc\x00", 2);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("uint32 0");
  s3->bytes = g_bytes_new ("\xcd\x00\x00", 3);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("uint32 0");
  s4->bytes = g_bytes_new ("\xce\x00\x00\x00\x00", 5);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("uint32 0");
  s5->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("uint32 0");
  s6->bytes = g_bytes_new ("\xd0\x00", 2);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("uint32 0");
  s7->bytes = g_bytes_new ("\xd1\x00\x00", 3);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("uint32 0");
  s8->bytes = g_bytes_new ("\xd2\x00\x00\x00\x00", 5);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("uint32 0");
  s9->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("uint32 1");
  s10->bytes = g_bytes_new ("\x01", 1);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("uint32 1");
  s11->bytes = g_bytes_new ("\xcc\x01", 2);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("uint32 1");
  s12->bytes = g_bytes_new ("\xcd\x00\x01", 3);
  list = g_list_append (list, s12);

  s13->variant = g_variant_new_parsed ("uint32 1");
  s13->bytes = g_bytes_new ("\xce\x00\x00\x00\x01", 5);
  list = g_list_append (list, s13);

  s14->variant = g_variant_new_parsed ("uint32 1");
  s14->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  list = g_list_append (list, s14);

  s15->variant = g_variant_new_parsed ("uint32 1");
  s15->bytes = g_bytes_new ("\xd0\x01", 2);
  list = g_list_append (list, s15);

  s16->variant = g_variant_new_parsed ("uint32 1");
  s16->bytes = g_bytes_new ("\xd1\x00\x01", 3);
  list = g_list_append (list, s16);

  s17->variant = g_variant_new_parsed ("uint32 1");
  s17->bytes = g_bytes_new ("\xd2\x00\x00\x00\x01", 5);
  list = g_list_append (list, s17);

  s18->variant = g_variant_new_parsed ("uint32 1");
  s18->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  list = g_list_append (list, s18);

  s19->variant = g_variant_new_parsed ("uint32 127");
  s19->bytes = g_bytes_new ("\x7f", 1);
  list = g_list_append (list, s19);

  s20->variant = g_variant_new_parsed ("uint32 127");
  s20->bytes = g_bytes_new ("\xcc\x7f", 2);
  list = g_list_append (list, s20);

  s21->variant = g_variant_new_parsed ("uint32 127");
  s21->bytes = g_bytes_new ("\xcd\x00\x7f", 3);
  list = g_list_append (list, s21);

  s22->variant = g_variant_new_parsed ("uint32 127");
  s22->bytes = g_bytes_new ("\xce\x00\x00\x00\x7f", 5);
  list = g_list_append (list, s22);

  s23->variant = g_variant_new_parsed ("uint32 127");
  s23->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  list = g_list_append (list, s23);

  s24->variant = g_variant_new_parsed ("uint32 127");
  s24->bytes = g_bytes_new ("\xd0\x7f", 2);
  list = g_list_append (list, s24);

  s25->variant = g_variant_new_parsed ("uint32 127");
  s25->bytes = g_bytes_new ("\xd1\x00\x7f", 3);
  list = g_list_append (list, s25);

  s26->variant = g_variant_new_parsed ("uint32 127");
  s26->bytes = g_bytes_new ("\xd2\x00\x00\x00\x7f", 5);
  list = g_list_append (list, s26);

  s27->variant = g_variant_new_parsed ("uint32 127");
  s27->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  list = g_list_append (list, s27);

  s28->variant = g_variant_new_parsed ("uint32 128");
  s28->bytes = g_bytes_new ("\xcc\x80", 2);
  list = g_list_append (list, s28);

  s29->variant = g_variant_new_parsed ("uint32 128");
  s29->bytes = g_bytes_new ("\xcd\x00\x80", 3);
  list = g_list_append (list, s29);

  s30->variant = g_variant_new_parsed ("uint32 128");
  s30->bytes = g_bytes_new ("\xce\x00\x00\x00\x80", 5);
  list = g_list_append (list, s30);

  s31->variant = g_variant_new_parsed ("uint32 128");
  s31->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x00\x80", 9);
  list = g_list_append (list, s31);

  s32->variant = g_variant_new_parsed ("uint32 128");
  s32->bytes = g_bytes_new ("\xd1\x00\x80", 3);
  list = g_list_append (list, s32);

  s33->variant = g_variant_new_parsed ("uint32 128");
  s33->bytes = g_bytes_new ("\xd2\x00\x00\x00\x80", 5);
  list = g_list_append (list, s33);

  s34->variant = g_variant_new_parsed ("uint32 128");
  s34->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x00\x80", 9);
  list = g_list_append (list, s34);

  s35->variant = g_variant_new_parsed ("uint32 255");
  s35->bytes = g_bytes_new ("\xcc\xff", 2);
  list = g_list_append (list, s35);

  s36->variant = g_variant_new_parsed ("uint32 255");
  s36->bytes = g_bytes_new ("\xcd\x00\xff", 3);
  list = g_list_append (list, s36);

  s37->variant = g_variant_new_parsed ("uint32 255");
  s37->bytes = g_bytes_new ("\xce\x00\x00\x00\xff", 5);
  list = g_list_append (list, s37);

  s38->variant = g_variant_new_parsed ("uint32 255");
  s38->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x00\xff", 9);
  list = g_list_append (list, s38);

  s39->variant = g_variant_new_parsed ("uint32 255");
  s39->bytes = g_bytes_new ("\xd1\x00\xff", 3);
  list = g_list_append (list, s39);

  s40->variant = g_variant_new_parsed ("uint32 255");
  s40->bytes = g_bytes_new ("\xd2\x00\x00\x00\xff", 5);
  list = g_list_append (list, s40);

  s41->variant = g_variant_new_parsed ("uint32 255");
  s41->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x00\xff", 9);
  list = g_list_append (list, s41);

  s42->variant = g_variant_new_parsed ("uint32 256");
  s42->bytes = g_bytes_new ("\xcd\x01\x00", 3);
  list = g_list_append (list, s42);

  s43->variant = g_variant_new_parsed ("uint32 256");
  s43->bytes = g_bytes_new ("\xce\x00\x00\x01\x00", 5);
  list = g_list_append (list, s43);

  s44->variant = g_variant_new_parsed ("uint32 256");
  s44->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  list = g_list_append (list, s44);

  s45->variant = g_variant_new_parsed ("uint32 256");
  s45->bytes = g_bytes_new ("\xd1\x01\x00", 3);
  list = g_list_append (list, s45);

  s46->variant = g_variant_new_parsed ("uint32 256");
  s46->bytes = g_bytes_new ("\xd2\x00\x00\x01\x00", 5);
  list = g_list_append (list, s46);

  s47->variant = g_variant_new_parsed ("uint32 256");
  s47->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  list = g_list_append (list, s47);

  s48->variant = g_variant_new_parsed ("uint32 65535");
  s48->bytes = g_bytes_new ("\xcd\xff\xff", 3);
  list = g_list_append (list, s48);

  s49->variant = g_variant_new_parsed ("uint32 65535");
  s49->bytes = g_bytes_new ("\xce\x00\x00\xff\xff", 5);
  list = g_list_append (list, s49);

  s50->variant = g_variant_new_parsed ("uint32 65535");
  s50->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  list = g_list_append (list, s50);

  s51->variant = g_variant_new_parsed ("uint32 65535");
  s51->bytes = g_bytes_new ("\xd2\x00\x00\xff\xff", 5);
  list = g_list_append (list, s51);

  s52->variant = g_variant_new_parsed ("uint32 65535");
  s52->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  list = g_list_append (list, s52);

  s53->variant = g_variant_new_parsed ("uint32 65536");
  s53->bytes = g_bytes_new ("\xce\x00\x01\x00\x00", 5);
  list = g_list_append (list, s53);

  s54->variant = g_variant_new_parsed ("uint32 65536");
  s54->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x00\x01\x00\x00", 9);
  list = g_list_append (list, s54);

  s55->variant = g_variant_new_parsed ("uint32 65536");
  s55->bytes = g_bytes_new ("\xd2\x00\x01\x00\x00", 5);
  list = g_list_append (list, s55);

  s56->variant = g_variant_new_parsed ("uint32 65536");
  s56->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x00\x01\x00\x00", 9);
  list = g_list_append (list, s56);

  s57->variant = g_variant_new_parsed ("uint32 2147483647");
  s57->bytes = g_bytes_new ("\xce\x7f\xff\xff\xff", 5);
  list = g_list_append (list, s57);

  s58->variant = g_variant_new_parsed ("uint32 2147483647");
  s58->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x7f\xff\xff\xff", 9);
  list = g_list_append (list, s58);

  s59->variant = g_variant_new_parsed ("uint32 2147483647");
  s59->bytes = g_bytes_new ("\xd2\x7f\xff\xff\xff", 5);
  list = g_list_append (list, s59);

  s60->variant = g_variant_new_parsed ("uint32 2147483647");
  s60->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x7f\xff\xff\xff", 9);
  list = g_list_append (list, s60);

  s61->variant = g_variant_new_parsed ("uint32 2147483648");
  s61->bytes = g_bytes_new ("\xce\x80\x00\x00\x00", 5);
  list = g_list_append (list, s61);

  s62->variant = g_variant_new_parsed ("uint32 2147483648");
  s62->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\x80\x00\x00\x00", 9);
  list = g_list_append (list, s62);

  s63->variant = g_variant_new_parsed ("uint32 2147483648");
  s63->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\x80\x00\x00\x00", 9);
  list = g_list_append (list, s63);

  s64->variant = g_variant_new_parsed ("uint32 4294967295");
  s64->bytes = g_bytes_new ("\xce\xff\xff\xff\xff", 5);
  list = g_list_append (list, s64);

  s65->variant = g_variant_new_parsed ("uint32 4294967295");
  s65->bytes = g_bytes_new ("\xcf\x00\x00\x00\x00\xff\xff\xff\xff", 9);
  list = g_list_append (list, s65);

  s66->variant = g_variant_new_parsed ("uint32 4294967295");
  s66->bytes = g_bytes_new ("\xd3\x00\x00\x00\x00\xff\xff\xff\xff", 9);
  list = g_list_append (list, s66);

  return list;
}

GList *
number_negative_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);
  Sample *s13 = g_slice_new0(Sample);
  Sample *s14 = g_slice_new0(Sample);
  Sample *s15 = g_slice_new0(Sample);
  Sample *s16 = g_slice_new0(Sample);
  Sample *s17 = g_slice_new0(Sample);
  Sample *s18 = g_slice_new0(Sample);
  Sample *s19 = g_slice_new0(Sample);
  Sample *s20 = g_slice_new0(Sample);
  Sample *s21 = g_slice_new0(Sample);
  Sample *s22 = g_slice_new0(Sample);
  Sample *s23 = g_slice_new0(Sample);
  Sample *s24 = g_slice_new0(Sample);
  Sample *s25 = g_slice_new0(Sample);
  Sample *s26 = g_slice_new0(Sample);
  Sample *s27 = g_slice_new0(Sample);
  Sample *s28 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("int32 -1");
  s1->bytes = g_bytes_new ("\xff", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("int32 -1");
  s2->bytes = g_bytes_new ("\xd0\xff", 2);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("int32 -1");
  s3->bytes = g_bytes_new ("\xd1\xff\xff", 3);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("int32 -1");
  s4->bytes = g_bytes_new ("\xd2\xff\xff\xff\xff", 5);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("int32 -1");
  s5->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\xff\xff", 9);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("int32 -32");
  s6->bytes = g_bytes_new ("\xe0", 1);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("int32 -32");
  s7->bytes = g_bytes_new ("\xd0\xe0", 2);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("int32 -32");
  s8->bytes = g_bytes_new ("\xd1\xff\xe0", 3);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("int32 -32");
  s9->bytes = g_bytes_new ("\xd2\xff\xff\xff\xe0", 5);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("int32 -32");
  s10->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\xff\xe0", 9);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("int32 -33");
  s11->bytes = g_bytes_new ("\xd0\xdf", 2);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("int32 -33");
  s12->bytes = g_bytes_new ("\xd1\xff\xdf", 3);
  list = g_list_append (list, s12);

  s13->variant = g_variant_new_parsed ("int32 -33");
  s13->bytes = g_bytes_new ("\xd2\xff\xff\xff\xdf", 5);
  list = g_list_append (list, s13);

  s14->variant = g_variant_new_parsed ("int32 -33");
  s14->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\xff\xdf", 9);
  list = g_list_append (list, s14);

  s15->variant = g_variant_new_parsed ("int32 -128");
  s15->bytes = g_bytes_new ("\xd0\x80", 2);
  list = g_list_append (list, s15);

  s16->variant = g_variant_new_parsed ("int32 -128");
  s16->bytes = g_bytes_new ("\xd1\xff\x80", 3);
  list = g_list_append (list, s16);

  s17->variant = g_variant_new_parsed ("int32 -128");
  s17->bytes = g_bytes_new ("\xd2\xff\xff\xff\x80", 5);
  list = g_list_append (list, s17);

  s18->variant = g_variant_new_parsed ("int32 -128");
  s18->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\xff\x80", 9);
  list = g_list_append (list, s18);

  s19->variant = g_variant_new_parsed ("int32 -256");
  s19->bytes = g_bytes_new ("\xd1\xff\x00", 3);
  list = g_list_append (list, s19);

  s20->variant = g_variant_new_parsed ("int32 -256");
  s20->bytes = g_bytes_new ("\xd2\xff\xff\xff\x00", 5);
  list = g_list_append (list, s20);

  s21->variant = g_variant_new_parsed ("int32 -256");
  s21->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\xff\x00", 9);
  list = g_list_append (list, s21);

  s22->variant = g_variant_new_parsed ("int32 -32768");
  s22->bytes = g_bytes_new ("\xd1\x80\x00", 3);
  list = g_list_append (list, s22);

  s23->variant = g_variant_new_parsed ("int32 -32768");
  s23->bytes = g_bytes_new ("\xd2\xff\xff\x80\x00", 5);
  list = g_list_append (list, s23);

  s24->variant = g_variant_new_parsed ("int32 -32768");
  s24->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\x80\x00", 9);
  list = g_list_append (list, s24);

  s25->variant = g_variant_new_parsed ("int32 -65536");
  s25->bytes = g_bytes_new ("\xd2\xff\xff\x00\x00", 5);
  list = g_list_append (list, s25);

  s26->variant = g_variant_new_parsed ("int32 -65536");
  s26->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\xff\xff\x00\x00", 9);
  list = g_list_append (list, s26);

  s27->variant = g_variant_new_parsed ("int32 -2147483648");
  s27->bytes = g_bytes_new ("\xd2\x80\x00\x00\x00", 5);
  list = g_list_append (list, s27);

  s28->variant = g_variant_new_parsed ("int32 -2147483648");
  s28->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\x80\x00\x00\x00", 9);
  list = g_list_append (list, s28);

  return list;
}

GList *
number_float_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("double 0.5");
  s1->bytes = g_bytes_new ("\xca\x3f\x00\x00\x00", 5);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("double 0.5");
  s2->bytes = g_bytes_new ("\xcb\x3f\xe0\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("double -0.5");
  s3->bytes = g_bytes_new ("\xca\xbf\x00\x00\x00", 5);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("double -0.5");
  s4->bytes = g_bytes_new ("\xcb\xbf\xe0\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s4);

  return list;
}

GList *
number_bignum_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("uint64 4294967296");
  s1->bytes = g_bytes_new ("\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("uint64 4294967296");
  s2->bytes = g_bytes_new ("\xd3\x00\x00\x00\x01\x00\x00\x00\x00", 9);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("int64 -4294967296");
  s3->bytes = g_bytes_new ("\xd3\xff\xff\xff\xff\x00\x00\x00\x00", 9);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("uint64 281474976710656");
  s4->bytes = g_bytes_new ("\xcf\x00\x01\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("uint64 281474976710656");
  s5->bytes = g_bytes_new ("\xd3\x00\x01\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("int64 -281474976710656");
  s6->bytes = g_bytes_new ("\xd3\xff\xff\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("uint64 9223372036854775807");
  s7->bytes = g_bytes_new ("\xd3\x7f\xff\xff\xff\xff\xff\xff\xff", 9);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("uint64 9223372036854775807");
  s8->bytes = g_bytes_new ("\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", 9);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("int64 -9223372036854775807");
  s9->bytes = g_bytes_new ("\xd3\x80\x00\x00\x00\x00\x00\x00\x01", 9);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("uint64 9223372036854775808");
  s10->bytes = g_bytes_new ("\xcf\x80\x00\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("int64 -9223372036854775808");
  s11->bytes = g_bytes_new ("\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 9);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("uint64 18446744073709551615");
  s12->bytes = g_bytes_new ("\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 9);
  list = g_list_append (list, s12);

  return list;
}

GList *
string_ascii_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);
  Sample *s13 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("''");
  s1->bytes = g_bytes_new ("\xa0", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("''");
  s2->bytes = g_bytes_new ("\xd9\x00", 2);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("''");
  s3->bytes = g_bytes_new ("\xda\x00\x00", 3);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("''");
  s4->bytes = g_bytes_new ("\xdb\x00\x00\x00\x00", 5);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("'a'");
  s5->bytes = g_bytes_new ("\xa1\x61", 2);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("'a'");
  s6->bytes = g_bytes_new ("\xd9\x01\x61", 3);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("'a'");
  s7->bytes = g_bytes_new ("\xda\x00\x01\x61", 4);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("'a'");
  s8->bytes = g_bytes_new ("\xdb\x00\x00\x00\x01\x61", 6);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("'1234567890123456789012345678901'");
  s9->bytes = g_bytes_new ("\xbf\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31", 32);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("'1234567890123456789012345678901'");
  s10->bytes = g_bytes_new ("\xd9\x1f\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31", 33);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("'1234567890123456789012345678901'");
  s11->bytes = g_bytes_new ("\xda\x00\x1f\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31", 34);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("'12345678901234567890123456789012'");
  s12->bytes = g_bytes_new ("\xd9\x20\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32", 34);
  list = g_list_append (list, s12);

  s13->variant = g_variant_new_parsed ("'12345678901234567890123456789012'");
  s13->bytes = g_bytes_new ("\xda\x00\x20\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32", 35);
  list = g_list_append (list, s13);

  return list;
}

GList *
string_utf8_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("'Кириллица'");
  s1->bytes = g_bytes_new ("\xb2\xd0\x9a\xd0\xb8\xd1\x80\xd0\xb8\xd0\xbb\xd0\xbb\xd0\xb8\xd1\x86\xd0\xb0", 19);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("'Кириллица'");
  s2->bytes = g_bytes_new ("\xd9\x12\xd0\x9a\xd0\xb8\xd1\x80\xd0\xb8\xd0\xbb\xd0\xbb\xd0\xb8\xd1\x86\xd0\xb0", 20);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("'ひらがな'");
  s3->bytes = g_bytes_new ("\xac\xe3\x81\xb2\xe3\x82\x89\xe3\x81\x8c\xe3\x81\xaa", 13);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("'ひらがな'");
  s4->bytes = g_bytes_new ("\xd9\x0c\xe3\x81\xb2\xe3\x82\x89\xe3\x81\x8c\xe3\x81\xaa", 14);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("'한글'");
  s5->bytes = g_bytes_new ("\xa6\xed\x95\x9c\xea\xb8\x80", 7);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("'한글'");
  s6->bytes = g_bytes_new ("\xd9\x06\xed\x95\x9c\xea\xb8\x80", 8);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("'汉字'");
  s7->bytes = g_bytes_new ("\xa6\xe6\xb1\x89\xe5\xad\x97", 7);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("'汉字'");
  s8->bytes = g_bytes_new ("\xd9\x06\xe6\xb1\x89\xe5\xad\x97", 8);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("'漢字'");
  s9->bytes = g_bytes_new ("\xa6\xe6\xbc\xa2\xe5\xad\x97", 7);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("'漢字'");
  s10->bytes = g_bytes_new ("\xd9\x06\xe6\xbc\xa2\xe5\xad\x97", 8);
  list = g_list_append (list, s10);

  return list;
}

GList *
string_emoji_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("'❤'");
  s1->bytes = g_bytes_new ("\xa3\xe2\x9d\xa4", 4);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("'❤'");
  s2->bytes = g_bytes_new ("\xd9\x03\xe2\x9d\xa4", 5);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("'🍺'");
  s3->bytes = g_bytes_new ("\xa4\xf0\x9f\x8d\xba", 5);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("'🍺'");
  s4->bytes = g_bytes_new ("\xd9\x04\xf0\x9f\x8d\xba", 6);
  list = g_list_append (list, s4);

  return list;
}

GList *
array_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);
  Sample *s13 = g_slice_new0(Sample);
  Sample *s14 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("@av []");
  s1->bytes = g_bytes_new ("\x90", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("@av []");
  s2->bytes = g_bytes_new ("\xdc\x00\x00", 3);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("@av []");
  s3->bytes = g_bytes_new ("\xdd\x00\x00\x00\x00", 5);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("@av [<uint32 1>]");
  s4->bytes = g_bytes_new ("\x91\x01", 2);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("@av [<uint32 1>]");
  s5->bytes = g_bytes_new ("\xdc\x00\x01\x01", 4);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("@av [<uint32 1>]");
  s6->bytes = g_bytes_new ("\xdd\x00\x00\x00\x01\x01", 6);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("@av [<uint32 1>, <uint32 2>, <uint32 3>, <uint32 4>, <uint32 5>, <uint32 6>, <uint32 7>, <uint32 8>, <uint32 9>, <uint32 10>, <uint32 11>, <uint32 12>, <uint32 13>, <uint32 14>, <uint32 15>]");
  s7->bytes = g_bytes_new ("\x9f\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 16);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("@av [<uint32 1>, <uint32 2>, <uint32 3>, <uint32 4>, <uint32 5>, <uint32 6>, <uint32 7>, <uint32 8>, <uint32 9>, <uint32 10>, <uint32 11>, <uint32 12>, <uint32 13>, <uint32 14>, <uint32 15>]");
  s8->bytes = g_bytes_new ("\xdc\x00\x0f\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 18);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("@av [<uint32 1>, <uint32 2>, <uint32 3>, <uint32 4>, <uint32 5>, <uint32 6>, <uint32 7>, <uint32 8>, <uint32 9>, <uint32 10>, <uint32 11>, <uint32 12>, <uint32 13>, <uint32 14>, <uint32 15>]");
  s9->bytes = g_bytes_new ("\xdd\x00\x00\x00\x0f\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 20);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("@av [<uint32 1>, <uint32 2>, <uint32 3>, <uint32 4>, <uint32 5>, <uint32 6>, <uint32 7>, <uint32 8>, <uint32 9>, <uint32 10>, <uint32 11>, <uint32 12>, <uint32 13>, <uint32 14>, <uint32 15>, <uint32 16>]");
  s10->bytes = g_bytes_new ("\xdc\x00\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10", 19);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("@av [<uint32 1>, <uint32 2>, <uint32 3>, <uint32 4>, <uint32 5>, <uint32 6>, <uint32 7>, <uint32 8>, <uint32 9>, <uint32 10>, <uint32 11>, <uint32 12>, <uint32 13>, <uint32 14>, <uint32 15>, <uint32 16>]");
  s11->bytes = g_bytes_new ("\xdd\x00\x00\x00\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10", 21);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("@av [<'a'>]");
  s12->bytes = g_bytes_new ("\x91\xa1\x61", 3);
  list = g_list_append (list, s12);

  s13->variant = g_variant_new_parsed ("@av [<'a'>]");
  s13->bytes = g_bytes_new ("\xdc\x00\x01\xa1\x61", 5);
  list = g_list_append (list, s13);

  s14->variant = g_variant_new_parsed ("@av [<'a'>]");
  s14->bytes = g_bytes_new ("\xdd\x00\x00\x00\x01\xa1\x61", 7);
  list = g_list_append (list, s14);

  return list;
}

GList *
map_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("@a(vv) []");
  s1->bytes = g_bytes_new ("\x80", 1);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("@a(vv) []");
  s2->bytes = g_bytes_new ("\xde\x00\x00", 3);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("@a(vv) []");
  s3->bytes = g_bytes_new ("\xdf\x00\x00\x00\x00", 5);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <uint32 1>)]");
  s4->bytes = g_bytes_new ("\x81\xa1\x61\x01", 4);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <uint32 1>)]");
  s5->bytes = g_bytes_new ("\xde\x00\x01\xa1\x61\x01", 6);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <uint32 1>)]");
  s6->bytes = g_bytes_new ("\xdf\x00\x00\x00\x01\xa1\x61\x01", 8);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <'A'>)]");
  s7->bytes = g_bytes_new ("\x81\xa1\x61\xa1\x41", 5);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <'A'>)]");
  s8->bytes = g_bytes_new ("\xde\x00\x01\xa1\x61\xa1\x41", 7);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <'A'>)]");
  s9->bytes = g_bytes_new ("\xdf\x00\x00\x00\x01\xa1\x61\xa1\x41", 9);
  list = g_list_append (list, s9);

  return list;
}

GList *
nested_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("@av [<@av []>]");
  s1->bytes = g_bytes_new ("\x91\x90", 2);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("@av [<@av []>]");
  s2->bytes = g_bytes_new ("\xdc\x00\x01\xdc\x00\x00", 6);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("@av [<@av []>]");
  s3->bytes = g_bytes_new ("\xdd\x00\x00\x00\x01\xdd\x00\x00\x00\x00", 10);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("@av [<@a(vv) []>]");
  s4->bytes = g_bytes_new ("\x91\x80", 2);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("@av [<@a(vv) []>]");
  s5->bytes = g_bytes_new ("\xdc\x00\x01\x80", 4);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("@av [<@a(vv) []>]");
  s6->bytes = g_bytes_new ("\xdd\x00\x00\x00\x01\x80", 6);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@a(vv) []>)]");
  s7->bytes = g_bytes_new ("\x81\xa1\x61\x80", 4);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@a(vv) []>)]");
  s8->bytes = g_bytes_new ("\xde\x00\x01\xa1\x61\xde\x00\x00", 8);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@a(vv) []>)]");
  s9->bytes = g_bytes_new ("\xdf\x00\x00\x00\x01\xa1\x61\xdf\x00\x00\x00\x00", 12);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@av []>)]");
  s10->bytes = g_bytes_new ("\x81\xa1\x61\x90", 4);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@av []>)]");
  s11->bytes = g_bytes_new ("\xde\x00\x01\xa1\x61\x90", 6);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("@a(vv) [(<'a'>, <@av []>)]");
  s12->bytes = g_bytes_new ("\xdf\x00\x00\x00\x01\xa1\x61\x90", 8);
  list = g_list_append (list, s12);

  return list;
}

GList *
timestamp_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);
  Sample *s12 = g_slice_new0(Sample);
  Sample *s13 = g_slice_new0(Sample);
  Sample *s14 = g_slice_new0(Sample);
  Sample *s15 = g_slice_new0(Sample);
  Sample *s16 = g_slice_new0(Sample);
  Sample *s17 = g_slice_new0(Sample);
  Sample *s18 = g_slice_new0(Sample);
  Sample *s19 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("[1514862245, 0]");
  s1->bytes = g_bytes_new ("\xd6\xff\x5a\x4a\xf6\xa5", 6);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("[1514862245, 678901234]");
  s2->bytes = g_bytes_new ("\xd7\xff\xa1\xdc\xd7\xc8\x5a\x4a\xf6\xa5", 10);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("[2147483647, 999999999]");
  s3->bytes = g_bytes_new ("\xd7\xff\xee\x6b\x27\xfc\x7f\xff\xff\xff", 10);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("[2147483648, 0]");
  s4->bytes = g_bytes_new ("\xd6\xff\x80\x00\x00\x00", 6);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("[2147483648, 1]");
  s5->bytes = g_bytes_new ("\xd7\xff\x00\x00\x00\x04\x80\x00\x00\x00", 10);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("[4294967295, 0]");
  s6->bytes = g_bytes_new ("\xd6\xff\xff\xff\xff\xff", 6);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("[4294967295, 999999999]");
  s7->bytes = g_bytes_new ("\xd7\xff\xee\x6b\x27\xfc\xff\xff\xff\xff", 10);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("[4294967296, 0]");
  s8->bytes = g_bytes_new ("\xd7\xff\x00\x00\x00\x01\x00\x00\x00\x00", 10);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("[17179869183, 999999999]");
  s9->bytes = g_bytes_new ("\xd7\xff\xee\x6b\x27\xff\xff\xff\xff\xff", 10);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("[17179869184, 0]");
  s10->bytes = g_bytes_new ("\xc7\x0c\xff\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00", 15);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("[-1, 0]");
  s11->bytes = g_bytes_new ("\xc7\x0c\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff", 15);
  list = g_list_append (list, s11);

  s12->variant = g_variant_new_parsed ("[-1, 999999999]");
  s12->bytes = g_bytes_new ("\xc7\x0c\xff\x3b\x9a\xc9\xff\xff\xff\xff\xff\xff\xff\xff\xff", 15);
  list = g_list_append (list, s12);

  s13->variant = g_variant_new_parsed ("[0, 0]");
  s13->bytes = g_bytes_new ("\xd6\xff\x00\x00\x00\x00", 6);
  list = g_list_append (list, s13);

  s14->variant = g_variant_new_parsed ("[0, 1]");
  s14->bytes = g_bytes_new ("\xd7\xff\x00\x00\x00\x04\x00\x00\x00\x00", 10);
  list = g_list_append (list, s14);

  s15->variant = g_variant_new_parsed ("[1, 0]");
  s15->bytes = g_bytes_new ("\xd6\xff\x00\x00\x00\x01", 6);
  list = g_list_append (list, s15);

  s16->variant = g_variant_new_parsed ("[-2208988801, 999999999]");
  s16->bytes = g_bytes_new ("\xc7\x0c\xff\x3b\x9a\xc9\xff\xff\xff\xff\xff\x7c\x55\x81\x7f", 15);
  list = g_list_append (list, s16);

  s17->variant = g_variant_new_parsed ("[-2208988800, 0]");
  s17->bytes = g_bytes_new ("\xc7\x0c\xff\x00\x00\x00\x00\xff\xff\xff\xff\x7c\x55\x81\x80", 15);
  list = g_list_append (list, s17);

  s18->variant = g_variant_new_parsed ("[-62167219200, 0]");
  s18->bytes = g_bytes_new ("\xc7\x0c\xff\x00\x00\x00\x00\xff\xff\xff\xf1\x86\x8b\x84\x00", 15);
  list = g_list_append (list, s18);

  s19->variant = g_variant_new_parsed ("[253402300799, 999999999]");
  s19->bytes = g_bytes_new ("\xc7\x0c\xff\x3b\x9a\xc9\xff\x00\x00\x00\x3a\xff\xf4\x41\x7f", 15);
  list = g_list_append (list, s19);

  return list;
}

GList *
ext_samples()
{
  GList *list = NULL;
  Sample *s1 = g_slice_new0(Sample);
  Sample *s2 = g_slice_new0(Sample);
  Sample *s3 = g_slice_new0(Sample);
  Sample *s4 = g_slice_new0(Sample);
  Sample *s5 = g_slice_new0(Sample);
  Sample *s6 = g_slice_new0(Sample);
  Sample *s7 = g_slice_new0(Sample);
  Sample *s8 = g_slice_new0(Sample);
  Sample *s9 = g_slice_new0(Sample);
  Sample *s10 = g_slice_new0(Sample);
  Sample *s11 = g_slice_new0(Sample);

  s1->variant = g_variant_new_parsed ("(1, @ay [0x10])");
  s1->bytes = g_bytes_new ("\xd4\x01\x10", 3);
  list = g_list_append (list, s1);

  s2->variant = g_variant_new_parsed ("(2, @ay [0x20, 0x21])");
  s2->bytes = g_bytes_new ("\xd5\x02\x20\x21", 4);
  list = g_list_append (list, s2);

  s3->variant = g_variant_new_parsed ("(3, @ay [0x30, 0x31, 0x32, 0x33])");
  s3->bytes = g_bytes_new ("\xd6\x03\x30\x31\x32\x33", 6);
  list = g_list_append (list, s3);

  s4->variant = g_variant_new_parsed ("(4, @ay [0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47])");
  s4->bytes = g_bytes_new ("\xd7\x04\x40\x41\x42\x43\x44\x45\x46\x47", 10);
  list = g_list_append (list, s4);

  s5->variant = g_variant_new_parsed ("(5, @ay [0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f])");
  s5->bytes = g_bytes_new ("\xd8\x05\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f", 18);
  list = g_list_append (list, s5);

  s6->variant = g_variant_new_parsed ("(6, @ay [])");
  s6->bytes = g_bytes_new ("\xc7\x00\x06", 3);
  list = g_list_append (list, s6);

  s7->variant = g_variant_new_parsed ("(6, @ay [])");
  s7->bytes = g_bytes_new ("\xc8\x00\x00\x06", 4);
  list = g_list_append (list, s7);

  s8->variant = g_variant_new_parsed ("(6, @ay [])");
  s8->bytes = g_bytes_new ("\xc9\x00\x00\x00\x00\x06", 6);
  list = g_list_append (list, s8);

  s9->variant = g_variant_new_parsed ("(7, @ay [0x70, 0x71, 0x72])");
  s9->bytes = g_bytes_new ("\xc7\x03\x07\x70\x71\x72", 6);
  list = g_list_append (list, s9);

  s10->variant = g_variant_new_parsed ("(7, @ay [0x70, 0x71, 0x72])");
  s10->bytes = g_bytes_new ("\xc8\x00\x03\x07\x70\x71\x72", 7);
  list = g_list_append (list, s10);

  s11->variant = g_variant_new_parsed ("(7, @ay [0x70, 0x71, 0x72])");
  s11->bytes = g_bytes_new ("\xc9\x00\x00\x00\x03\x07\x70\x71\x72", 9);
  list = g_list_append (list, s11);

  return list;
}

