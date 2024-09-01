/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2023 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence
 * (EUPL) v.1.2 and is subject to its terms as set out below.
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
#include "../exceptions.h"
#include "../float.h"
#include "../string.h"
#include "../coordinate.h"

/*
 * fiftyoneDegreesCoordinate extract coordinate from String item test
 */
TEST(Coordinate, Coordinate_ItemToCoordinate_Positive)
{
	const fiftyoneDegreesCoordinate expectedCoordinate = { 1.2f, 3.4f };
	fiftyoneDegreesCoordinate resultCoordinate;
	
	fiftyoneDegreesString string;
	string.size = 9;
	string.value = FIFTYONE_DEGREES_STRING_COORDINATE;
	string.trail.coordinate.lat = 
		FIFTYONE_DEGREES_NATIVE_TO_FLOAT(expectedCoordinate.lat);
	string.trail.coordinate.lon = 
		FIFTYONE_DEGREES_NATIVE_TO_FLOAT(expectedCoordinate.lon);

	fiftyoneDegreesCollectionItem item;
	item.data.ptr = (byte *)&string;

	FIFTYONE_DEGREES_EXCEPTION_CREATE
	resultCoordinate = fiftyoneDegreesIpiGetCoordinate(&item, exception);
	EXPECT_TRUE(FIFTYONE_DEGREES_EXCEPTION_OKAY) << "No exception should "
		"be thrown at this point.";
	EXPECT_TRUE(resultCoordinate.lat == expectedCoordinate.lat
		&& resultCoordinate.lon == expectedCoordinate.lon) << "The actual "
		"coordinate is not the same as what being expected.";
}