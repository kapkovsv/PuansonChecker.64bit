#pragma once

struct ExchangeData {
	struct AxisData {
		int32_t cSteps;
		int32_t targetPos;
		uint32_t findReference;
		uint32_t state;
		int32_t pos;
		int32_t d;
		int32_t minSlack;
	} axis[2];
	uint32_t data[0x10];
};
