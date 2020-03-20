/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2019 51 Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY.
 *
 * This Original Work is licensed under the European Union Public Licence (EUPL) 
 * v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 * 
 * If using the Work as, or as part of, a network application, by 
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading, 
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */
 
#include "pch.h"
#include "../threading.h"
#include "Base.hpp"

typedef union singleWidthUnion_t {
    int count;
    void *padding;
} singleWidthUnion;

typedef struct doubleWidth_t {
    void *ptr;
    singleWidthUnion count;
} doubleWidth;

class Threading : public Base
{
public:
    doubleWidth item;

protected:

	void SetUp() {
		Base::SetUp();
	}

	void TearDown() {
		Base::TearDown();
	}
};
TEST_F(Threading, DoubleWidthExchange_Matching) {

    int val1 = 122;
    int val2 = 123;


    item.count.padding = nullptr;
    item.ptr = (void*)&val1;

    doubleWidth newItem;
    newItem = item;
    doubleWidth compare;
    compare = newItem;
    newItem.ptr = (void*)&val2;
    newItem.count.count++;

    bool changed = FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_DW(&item, newItem, compare);
    
    ASSERT_TRUE(changed);
    ASSERT_EQ(newItem.count.count, item.count.count);
    ASSERT_EQ(newItem.ptr, item.ptr);
    ASSERT_EQ(*(int*)newItem.ptr, *(int*)item.ptr);
}
TEST_F(Threading, DoubleWidthExchange_NonMatching) {

    int val1 = 122;
    int val2 = 123;

    item.count.padding = nullptr;
    item.ptr = (void*)&val1;

    doubleWidth newItem;
    newItem = item;
    doubleWidth compare;
    compare = newItem;
    newItem.ptr = (void*)&val2;
    newItem.count.count++;

    item.count.count++;

    bool changed = FIFTYONE_DEGREES_INTERLOCK_EXCHANGE_DW(&item, newItem, compare);
    ASSERT_FALSE(changed);
    ASSERT_NE(newItem.ptr, item.ptr);
    ASSERT_NE(*(int*)newItem.ptr, *(int*)item.ptr);
}