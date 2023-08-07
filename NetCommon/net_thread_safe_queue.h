#pragma once

#include "net_common.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class tsqueue
		{
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete;

			// Returns and maintains item at front of Queue
			const T& front()
			{
				// Protect Queue access
				std::scoped_lock lock(_mutex);
				return _deque.front();
			}

			// Returns and maintains item at back of Queue
			const T& back()
			{
				std::scoped_lock lock(_mutex);
				return _deque.back();
			}

			// Adds an item to back of Queue
			void push_back(const T& item)
			{
				std::scoped_lock lock(_mutex);
				_deque.emplace_back(std::move(item));

				std::unique_lock<std::mutex> ul(_mutex);
				_condition.notify_one();
			}

			// Adds an item to front of Queue
			void push_front(const T& item)
			{
				std::scoped_lock lock(_mutex);
				_deque.emplace_front(std::move(item));

				std::unique_lock<std::mutex> ul(_mutex);
				_condition.notify_one();
			}

			// Returns true if Queue has no items
			bool empty()
			{
				std::scoped_lock lock(_mutex);
				return _deque.empty();
			}

			// Returns number of items in Queue
			size_t count()
			{
				std::scoped_lock lock(_mutex);
				return _deque.size();
			}

			// Clears Queue
			void clear()
			{
				std::scoped_lock lock(_mutex);
				_deque.clear();
			}

			// Removes and returns item from front of Queue
			T pop_front()
			{
				std::scoped_lock lock(_mutex);
				auto t = std::move(_deque.front());
				_deque.pop_front();
				return t;
			}

			// Removes and returns item from back of Queue
			T pop_back()
			{
				std::scoped_lock lock(_mutex);
				auto t = std::move(_deque.back());
				_deque.pop_back();
				return t;
			}

			virtual ~tsqueue() { clear(); }

		protected:
			std::mutex _mutex;
			std::deque<T> _deque;
		};
	}
}