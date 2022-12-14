// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_field.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/field_types.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;

namespace autofill {
namespace {

// Returns a FormFieldData object corresponding to a <select> field populated
// with the given |options|.
FormFieldData GenerateSelectFieldWithOptions(const char* const* options,
                                             size_t options_size) {
  std::vector<base::string16> options16(options_size);
  for (size_t i = 0; i < options_size; ++i)
    options16[i] = UTF8ToUTF16(options[i]);

  FormFieldData form_field;
  form_field.form_control_type = "select-one";
  form_field.option_values = options16;
  form_field.option_contents = options16;
  return form_field;
}

struct TestCase {
  std::string card_number_;
  size_t total_spilts_;
  std::vector<int> splits_;
  std::vector<std::string> expected_results_;
};

// Returns the offset to be set within the credit card number field.
size_t GetNumberOffset(size_t index, const TestCase& test) {
  size_t result = 0;
  for (size_t i = 0; i < index; ++i)
    result += test.splits_[i];
  return result;
}

TEST(AutofillFieldTest, Type) {
  AutofillField field;
  ASSERT_EQ(NO_SERVER_DATA, field.server_type());
  ASSERT_EQ(UNKNOWN_TYPE, field.heuristic_type());

  // |server_type_| is NO_SERVER_DATA, so |heuristic_type_| is returned.
  EXPECT_EQ(UNKNOWN_TYPE, field.Type().GetStorableType());

  // Set the heuristic type and check it.
  field.set_heuristic_type(NAME_FIRST);
  EXPECT_EQ(NAME_FIRST, field.Type().GetStorableType());
  EXPECT_EQ(NAME, field.Type().group());

  // Set the server type and check it.
  field.set_server_type(ADDRESS_BILLING_LINE1);
  EXPECT_EQ(ADDRESS_HOME_LINE1, field.Type().GetStorableType());
  EXPECT_EQ(ADDRESS_BILLING, field.Type().group());

  // Remove the server type to make sure the heuristic type is preserved.
  field.set_server_type(NO_SERVER_DATA);
  EXPECT_EQ(NAME_FIRST, field.Type().GetStorableType());
  EXPECT_EQ(NAME, field.Type().group());
}

TEST(AutofillFieldTest, IsEmpty) {
  AutofillField field;
  ASSERT_EQ(base::string16(), field.value);

  // Field value is empty.
  EXPECT_TRUE(field.IsEmpty());

  // Field value is non-empty.
  field.value = ASCIIToUTF16("Value");
  EXPECT_FALSE(field.IsEmpty());
}

TEST(AutofillFieldTest, FieldSignature) {
  AutofillField field;
  ASSERT_EQ(base::string16(), field.name);
  ASSERT_EQ(std::string(), field.form_control_type);

  // Signature is empty.
  EXPECT_EQ("2085434232", field.FieldSignature());

  // Field name is set.
  field.name = ASCIIToUTF16("Name");
  EXPECT_EQ("1606968241", field.FieldSignature());

  // Field form control type is set.
  field.form_control_type = "text";
  EXPECT_EQ("502192749", field.FieldSignature());

  // Heuristic type does not affect FieldSignature.
  field.set_heuristic_type(NAME_FIRST);
  EXPECT_EQ("502192749", field.FieldSignature());

  // Server type does not affect FieldSignature.
  field.set_server_type(NAME_LAST);
  EXPECT_EQ("502192749", field.FieldSignature());
}

TEST(AutofillFieldTest, IsFieldFillable) {
  AutofillField field;
  ASSERT_EQ(UNKNOWN_TYPE, field.Type().GetStorableType());

  // Type is unknown.
  EXPECT_FALSE(field.IsFieldFillable());

  // Only heuristic type is set.
  field.set_heuristic_type(NAME_FIRST);
  EXPECT_TRUE(field.IsFieldFillable());

  // Only server type is set.
  field.set_heuristic_type(UNKNOWN_TYPE);
  field.set_server_type(NAME_LAST);
  EXPECT_TRUE(field.IsFieldFillable());

  // Both types set.
  field.set_heuristic_type(NAME_FIRST);
  field.set_server_type(NAME_LAST);
  EXPECT_TRUE(field.IsFieldFillable());

  // Field has autocomplete="off" set. Chrome ignores the attribute.
  field.should_autocomplete = false;
  EXPECT_TRUE(field.IsFieldFillable());
}

TEST(AutofillFieldTest, FillPhoneNumber) {
  AutofillField field;
  field.SetHtmlType(HTML_TYPE_TEL_LOCAL_PREFIX, HtmlFieldMode());

  // Fill with a non-phone number; should fill normally.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("Oh hai"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Oh hai"), field.value);

  // Fill with a phone number; should fill just the prefix.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("5551234"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("555"), field.value);

  // Now reset the type, and set a max-length instead.
  field.SetHtmlType(HTML_TYPE_UNKNOWN, HtmlFieldMode());
  field.set_heuristic_type(PHONE_HOME_NUMBER);
  field.max_length = 4;

  // Fill with a phone-number; should fill just the suffix.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("5551234"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("1234"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlByValue) {
  const char* const kOptions[] = {
    "Eenie", "Meenie", "Miney", "Mo",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kOptions, arraysize(kOptions)),
      base::string16());

  // Set semantically empty contents for each option, so that only the values
  // can be used for matching.
  for (size_t i = 0; i < field.option_contents.size(); ++i)
    field.option_contents[i] = base::SizeTToString16(i);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("Meenie"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Meenie"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlByContents) {
  const char* const kOptions[] = {
    "Eenie", "Meenie", "Miney", "Mo",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kOptions, arraysize(kOptions)),
      base::string16());

  // Set semantically empty values for each option, so that only the contents
  // can be used for matching.
  for (size_t i = 0; i < field.option_values.size(); ++i)
    field.option_values[i] = base::SizeTToString16(i);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("Miney"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("2"), field.value);  // Corresponds to "Miney".
}

TEST(AutofillFieldTest, FillSelectControlWithFullCountryNames) {
  const char* const kCountries[] = {
    "Albania", "Canada"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kCountries, arraysize(kCountries)),
      base::string16());
  field.set_heuristic_type(ADDRESS_HOME_COUNTRY);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("CA"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Canada"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithAbbreviatedCountryNames) {
  const char* const kCountries[] = {
    "AL", "CA"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kCountries, arraysize(kCountries)),
      base::string16());
  field.set_heuristic_type(ADDRESS_HOME_COUNTRY);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("Canada"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("CA"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithFullStateNames) {
  const char* const kStates[] = {
    "Alabama", "California"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
      base::string16());
  field.set_heuristic_type(ADDRESS_HOME_STATE);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("CA"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("California"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithAbbreviateStateNames) {
  const char* const kStates[] = {
    "AL", "CA"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
      base::string16());
  field.set_heuristic_type(ADDRESS_HOME_STATE);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("California"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("CA"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithInexactFullStateNames) {
  {
    const char* const kStates[] = {
      "SC - South Carolina", "CA - California", "NC - North Carolina",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("California"), "en-US", "en-US", &field);
    EXPECT_EQ(ASCIIToUTF16("CA - California"), field.value);
  }

  // Don't accidentally match "Virginia" to "West Virginia".
  {
    const char* const kStates[] = {
      "WV - West Virginia", "VA - Virginia", "NV - North Virginia",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("Virginia"), "en-US", "en-US", &field);
    EXPECT_EQ(ASCIIToUTF16("VA - Virginia"), field.value);
  }

  // Do accidentally match "Virginia" to "West Virginia". NB: Ideally, Chrome
  // would fail this test. It's here to document behavior rather than enforce
  // it.
  {
    const char* const kStates[] = {
      "WV - West Virginia", "TX - Texas",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("Virginia"), "en-US", "en-US", &field);
    EXPECT_EQ(ASCIIToUTF16("WV - West Virginia"), field.value);
  }

  // Tests that substring matches work for full state names (a full token
  // match isn't required). Also tests that matches work for states with
  // whitespace in the middle.
  {
    const char* const kStates[] = {
      "California.", "North Carolina.",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("North Carolina"), "en-US", "en-US", &field);
    EXPECT_EQ(ASCIIToUTF16("North Carolina."), field.value);
  }
}

TEST(AutofillFieldTest, FillSelectControlWithInexactAbbreviations) {
  {
    const char* const kStates[] = {
      "NC - North Carolina", "CA - California",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kStates, arraysize(kStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("CA"), "en-US", "en-US", &field);
    EXPECT_EQ(ASCIIToUTF16("CA - California"), field.value);
  }

  {
    const char* const kNotStates[] = {
      "NCNCA", "SCNCA",
    };
    AutofillField field(
        GenerateSelectFieldWithOptions(kNotStates, arraysize(kNotStates)),
        base::string16());
    field.set_heuristic_type(ADDRESS_HOME_STATE);

    AutofillField::FillFormField(
        field, ASCIIToUTF16("NC"), "en-US", "en-US", &field);
    EXPECT_EQ(base::string16(), field.value);
  }
}

TEST(AutofillFieldTest, FillSelectControlWithNumericMonth) {
  const char* const kMonthsNumeric[] = {
    "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kMonthsNumeric, arraysize(kMonthsNumeric)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_MONTH);

  // Try with a leading zero.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("03"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("03"), field.value);

  // Try without a leading zero.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("4"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("04"), field.value);

  // Try a two-digit month.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("11"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("11"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithAbbreviatedMonthName) {
  const char* const kMonthsAbbreviated[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(
          kMonthsAbbreviated, arraysize(kMonthsAbbreviated)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_MONTH);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("04"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Apr"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithFullMonthName) {
  const char* const kMonthsFull[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kMonthsFull, arraysize(kMonthsFull)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_MONTH);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("04"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("April"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithFrenchMonthName) {
  const char* const kMonthsFrench[] = {
    "JANV", "F??VR.", "MARS", "d??cembre"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kMonthsFrench, arraysize(kMonthsFrench)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_MONTH);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("02"), "fr-FR", "fr-FR", &field);
  EXPECT_EQ(UTF8ToUTF16("F??VR."), field.value);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("01"), "fr-FR", "fr-FR", &field);
  EXPECT_EQ(UTF8ToUTF16("JANV"), field.value);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("12"), "fr-FR", "fr-FR", &field);
  EXPECT_EQ(UTF8ToUTF16("d??cembre"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithNumericMonthSansLeadingZero) {
  const char* const kMonthsNumeric[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(kMonthsNumeric, arraysize(kMonthsNumeric)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_MONTH);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("04"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("4"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithTwoDigitCreditCardYear) {
  const char* const kYears[] = {
    "12", "13", "14", "15", "16", "17", "18", "19"
  };
  AutofillField field(GenerateSelectFieldWithOptions(kYears, arraysize(kYears)),
                      base::string16());
  field.set_heuristic_type(CREDIT_CARD_EXP_2_DIGIT_YEAR);

  AutofillField::FillFormField(
      field, ASCIIToUTF16("2017"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("17"), field.value);
}

TEST(AutofillFieldTest, FillSelectControlWithCreditCardType) {
  const char* const kCreditCardTypes[] = {
    "Visa", "Master Card", "AmEx", "discover"
  };
  AutofillField field(
      GenerateSelectFieldWithOptions(
          kCreditCardTypes, arraysize(kCreditCardTypes)),
      base::string16());
  field.set_heuristic_type(CREDIT_CARD_TYPE);

  // Normal case:
  AutofillField::FillFormField(
      field, ASCIIToUTF16("Visa"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Visa"), field.value);

  // Filling should be able to handle intervening whitespace:
  AutofillField::FillFormField(
      field, ASCIIToUTF16("MasterCard"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("Master Card"), field.value);

  // American Express is sometimes abbreviated as AmEx:
  AutofillField::FillFormField(
      field, ASCIIToUTF16("American Express"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("AmEx"), field.value);

  // Case insensitivity:
  AutofillField::FillFormField(
      field, ASCIIToUTF16("Discover"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("discover"), field.value);
}

TEST(AutofillFieldTest, FillMonthControl) {
  AutofillField field;
  field.form_control_type = "month";

  // Try a month with two digits.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("12/2017"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("2017-12"), field.value);

  // Try a month with a leading zero.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("03/2019"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("2019-03"), field.value);

  // Try a month without a leading zero.
  AutofillField::FillFormField(
      field, ASCIIToUTF16("4/2018"), "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("2018-04"), field.value);
}

TEST(AutofillFieldTest, FillStreetAddressTextArea) {
  AutofillField field;
  field.form_control_type = "textarea";

  base::string16 value = ASCIIToUTF16("123 Fake St.\n"
                                      "Apt. 42");
  AutofillField::FillFormField(field, value, "en-US", "en-US", &field);
  EXPECT_EQ(value, field.value);

  base::string16 ja_value = UTF8ToUTF16("?????????26-1\n"
                                        "????????????????????????6???");
  AutofillField::FillFormField(field, ja_value, "ja-JP", "en-US", &field);
  EXPECT_EQ(ja_value, field.value);
}

TEST(AutofillFieldTest, FillStreetAddressTextField) {
  AutofillField field;
  field.form_control_type = "text";
  field.set_server_type(ADDRESS_HOME_STREET_ADDRESS);

  base::string16 value = ASCIIToUTF16("123 Fake St.\n"
                                      "Apt. 42");
  AutofillField::FillFormField(field, value, "en-US", "en-US", &field);
  EXPECT_EQ(ASCIIToUTF16("123 Fake St., Apt. 42"), field.value);

  AutofillField::FillFormField(field,
                               UTF8ToUTF16("?????????26-1\n"
                                           "????????????????????????6???"),
                               "ja-JP",
                               "en-US",
                               &field);
  EXPECT_EQ(UTF8ToUTF16("?????????26-1????????????????????????6???"), field.value);
}

TEST(AutofillFieldTest, FillCreditCardNumberWithoutSplits) {
  // Case 1: card number without any spilt.
  AutofillField cc_number_full;
  cc_number_full.set_heuristic_type(CREDIT_CARD_NUMBER);
  AutofillField::FillFormField(cc_number_full,
                               ASCIIToUTF16("4111111111111111"),
                               "en-US",
                               "en-US",
                               &cc_number_full);

  // Verify that full card-number shall get fill properly.
  EXPECT_EQ(ASCIIToUTF16("4111111111111111"), cc_number_full.value);
  EXPECT_EQ(0U, cc_number_full.credit_card_number_offset());
}

TEST(AutofillFieldTest, FillCreditCardNumberWithEqualSizeSplits) {
  // Case 2: card number broken up into four equal groups, of length 4.
  TestCase test;
  test.card_number_ = "5187654321098765";
  test.total_spilts_ = 4;
  int splits[] = {4, 4, 4, 4};
  test.splits_ = std::vector<int>(splits, splits + arraysize(splits));
  std::string results[] = {"5187", "6543", "2109", "8765"};
  test.expected_results_ =
      std::vector<std::string>(results, results + arraysize(results));

  for (size_t i = 0; i < test.total_spilts_; ++i) {
    AutofillField cc_number_part;
    cc_number_part.set_heuristic_type(CREDIT_CARD_NUMBER);
    cc_number_part.max_length = test.splits_[i];
    cc_number_part.set_credit_card_number_offset(4 * i);

    // Fill with a card-number; should fill just the card_number_part.
    AutofillField::FillFormField(cc_number_part,
                                 ASCIIToUTF16(test.card_number_),
                                 "en-US",
                                 "en-US",
                                 &cc_number_part);

    // Verify for expected results.
    EXPECT_EQ(ASCIIToUTF16(test.expected_results_[i]),
              cc_number_part.value.substr(0, cc_number_part.max_length));
    EXPECT_EQ(4 * i, cc_number_part.credit_card_number_offset());
  }

  // Verify that full card-number shall get fill properly as well.
  AutofillField cc_number_full;
  cc_number_full.set_heuristic_type(CREDIT_CARD_NUMBER);
  AutofillField::FillFormField(cc_number_full,
                               ASCIIToUTF16(test.card_number_),
                               "en-US",
                               "en-US",
                               &cc_number_full);

  // Verify for expected results.
  EXPECT_EQ(ASCIIToUTF16(test.card_number_), cc_number_full.value);
}

TEST(AutofillFieldTest, FillCreditCardNumberWithUnequalSizeSplits) {
  // Case 3: card with 15 digits number, broken up into three unequal groups, of
  // lengths 4, 6, and 5.
  TestCase test;
  test.card_number_ = "423456789012345";
  test.total_spilts_ = 3;
  int splits[] = {4, 6, 5};
  test.splits_ = std::vector<int>(splits, splits + arraysize(splits));
  std::string results[] = {"4234", "567890", "12345"};
  test.expected_results_ =
      std::vector<std::string>(results, results + arraysize(results));

  // Start executing test cases to verify parts and full credit card number.
  for (size_t i = 0; i < test.total_spilts_; ++i) {
    AutofillField cc_number_part;
    cc_number_part.set_heuristic_type(CREDIT_CARD_NUMBER);
    cc_number_part.max_length = test.splits_[i];
    cc_number_part.set_credit_card_number_offset(GetNumberOffset(i, test));

    // Fill with a card-number; should fill just the card_number_part.
    AutofillField::FillFormField(cc_number_part,
                                 ASCIIToUTF16(test.card_number_),
                                 "en-US",
                                 "en-US",
                                 &cc_number_part);

    // Verify for expected results.
    EXPECT_EQ(ASCIIToUTF16(test.expected_results_[i]),
              cc_number_part.value.substr(0, cc_number_part.max_length));
    EXPECT_EQ(GetNumberOffset(i, test),
              cc_number_part.credit_card_number_offset());
  }

  // Verify that full card-number shall get fill properly as well.
  AutofillField cc_number_full;
  cc_number_full.set_heuristic_type(CREDIT_CARD_NUMBER);
  AutofillField::FillFormField(cc_number_full,
                               ASCIIToUTF16(test.card_number_),
                               "en-US",
                               "en-US",
                               &cc_number_full);

  // Verify for expected results.
  EXPECT_EQ(ASCIIToUTF16(test.card_number_), cc_number_full.value);
}

TEST(AutofillFieldTest, FindValueInSelectControl) {
  const size_t kBadIndex = 1000;

  {
    const char* const kCountries[] = {
      "Albania", "Canada"
    };
    FormFieldData field(
        GenerateSelectFieldWithOptions(kCountries, arraysize(kCountries)));
    size_t index = kBadIndex;
    bool ret = AutofillField::FindValueInSelectControl(
        field, ASCIIToUTF16("Canada"), &index);
    EXPECT_TRUE(ret);
    EXPECT_EQ(1U, index);

    index = kBadIndex;
    ret = AutofillField::FindValueInSelectControl(
        field, ASCIIToUTF16("CANADA"), &index);
    EXPECT_TRUE(ret);
    EXPECT_EQ(1U, index);

    index = kBadIndex;
    ret = AutofillField::FindValueInSelectControl(
        field, ASCIIToUTF16("Canadia"), &index);
    EXPECT_FALSE(ret);
    EXPECT_EQ(kBadIndex, index);
  }

  {
    const char* const kProvinces[] = {
      "ALBERTA", "QU??BEC", "NOVA SCOTIA",
    };
    FormFieldData field(
        GenerateSelectFieldWithOptions(kProvinces, arraysize(kProvinces)));
    size_t index = kBadIndex;
    bool ret = AutofillField::FindValueInSelectControl(
        field, ASCIIToUTF16("alberta"), &index);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0U, index);

    index = kBadIndex;
    ret = AutofillField::FindValueInSelectControl(
        field, UTF8ToUTF16("qu??bec"), &index);
    EXPECT_TRUE(ret);
    EXPECT_EQ(1U, index);

    index = kBadIndex;
    ret = AutofillField::FindValueInSelectControl(
        field, UTF8ToUTF16("NoVaScOtIa"), &index);
    EXPECT_TRUE(ret);
    EXPECT_EQ(2U, index);
  }
}

}  // namespace
}  // namespace autofill
