//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************

#pragma once
#include <vector>
#include <functional>
#include <array>
#include "klog.hpp"
using namespace klog; 
namespace knet {
namespace utils {

template <typename T = char, std::size_t kMaxSize = 10240>
class LoopBuffer {
public:
	using DataHandler = std::function<void(const T*, uint32_t)>;

	bool push_back(const T* data, uint32_t len) {
		int32_t tailPos = tail_pos;
		uint32_t disSize = tailPos + kMaxSize - head_pos;
		uint32_t usedSize = disSize >= kMaxSize ? (disSize - kMaxSize) : disSize;
		// ilog("push length is {}  used size {}  ", len,  usedSize);
		if (len + usedSize > kMaxSize - 1) {
			wlog("push failed   length is {}  used size {}  ", len, usedSize);
			return false;
		}
		if (tailPos + len > kMaxSize) {
			uint32_t tailLeft = kMaxSize - tailPos;
			std::copy(data, data + tailLeft, buffer.begin() + tailPos);
			std::copy(data + tailLeft, data + len, buffer.begin());
			tailPos = len - tailLeft;
		} else {
			std::copy(data, data + len, buffer.begin() + tailPos);
			tailPos += len;
			tailPos = tailPos % kMaxSize;
		}
		tail_pos = tailPos;
		return true;
	}

	bool push_one(T& item) {
		uint32_t tailPos = tail_pos;
		uint32_t disSize = tailPos + kMaxSize - head_pos;
		uint32_t usedSize = disSize >= kMaxSize ? (disSize - kMaxSize) : disSize;
		if (usedSize + 1 > kMaxSize - 1) {
			wlog("push failed   length is {}  used size {}  ", 1, usedSize);
			return false;
		}
		buffer[tail_pos] = item;
		tail_pos = (tail_pos + 1) % kMaxSize;
		return true;
	}

	T pop_one() {
		uint32_t headPos = head_pos;
		uint32_t disSize = tail_pos + kMaxSize - headPos;
		uint32_t usedSize = disSize >= kMaxSize ? (disSize - kMaxSize) : disSize;
		if (usedSize > 1) {
			int32_t oPos = head_pos;
			head_pos = (head_pos + 1) % kMaxSize;
			return buffer[oPos];
		}

		return T();
	}

	int32_t pop_front(uint32_t len = 1, DataHandler handler = nullptr) {

		int32_t headPos = head_pos;
		int32_t disSize = tail_pos + kMaxSize - headPos;
		int32_t usedSize = disSize >= kMaxSize ? (disSize - kMaxSize) : disSize;
		if (len == 0) {
			len = usedSize;
		}

		ilog("pop  length is {}  used size {}  ", len, usedSize);
		if (len > usedSize) {
			elog("pop  failed length is {}  used size {}  ", len, usedSize);
			return 0;
		}
		std::vector<T> popData(len);
		if (headPos + len > kMaxSize) {
			uint32_t tailLeft = kMaxSize - headPos;
			if (headPos < kMaxSize) {
				std::copy(buffer.begin() + headPos, buffer.begin() + kMaxSize, popData.begin());
			}
			std::copy(buffer.begin(), buffer.begin() + (len - tailLeft), popData.end());
			headPos = len - tailLeft;
		} else {
			std::copy(buffer.begin() + headPos, buffer.begin() + headPos + len, popData.begin());
			headPos += len;
			headPos = headPos % kMaxSize;
		}

		if (handler) {
			handler(popData.data(), popData.size());
		}
		head_pos = headPos;
		return len;
	};

	bool empty() { return head_pos == tail_pos; }

	int32_t pop_segment(DataHandler handler = nullptr) {
		int32_t headPos = head_pos;
		int32_t tailPos = tail_pos; // tail_pos may change in pop thread
		tailPos = (tailPos >= headPos) ? tailPos : kMaxSize;
		if (tailPos > headPos) {
			handler(buffer.data() + headPos, tailPos - headPos);
		} else {
			elog("pop segment is 0,  head {}  tail {}", headPos, tailPos);
			return 0;
		}
		return tailPos - headPos;
	};

	bool drop_front(uint32_t len) {
		head_pos = (head_pos + len) % kMaxSize;
		return true;
	}

	// bool drop_front(uint32_t len) {
	//	int32_t headPos = head_pos + len ;
	//	int32_t tailPos = tail_pos;
	//	if (tailPos + kMaxSize >= headPos)
	//	{
	//		head_pos = (headPos >=  kMaxSize)? (headPos - kMaxSize):headPos;
	//		return true;
	//	}
	//	return false;
	//}

private:
	volatile uint32_t head_pos = 0;
	volatile uint32_t tail_pos = 0;
	//		uint32_t head_pos = 0;
	//		uint32_t tail_pos = 0;
	std::array<T, kMaxSize + 1> buffer;
};

} // namespace utils
} // namespace gnet
