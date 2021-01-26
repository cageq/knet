//***************************************************************
//	created:	2020/08/01
//	author:		arthur wong
//***************************************************************

#pragma once
#include <thread>
#include <mutex>
namespace knet
{

	namespace utils
	{

		template <typename T>
		class Singleton
		{

			Singleton(const Singleton &) = delete;
			Singleton &operator=(const Singleton &) = delete;

		public:
			Singleton() {}
			~Singleton()
			{
				if (s_ins)
				{
					delete s_ins;
					s_ins = nullptr;
				}
			}

			static T *pointer()
			{
				if (s_ins)
				{
					return s_ins;
				}
				std::call_once(s_once, [&]() {
					s_ins = new T();
				});
				return s_ins;
			}

			template <class... P>
			static T &instance(P... args)
			{
				if (s_ins)
				{
					return *s_ins;
				}

				std::call_once(s_once, [&]() {
					s_ins = new T(args...);
				});
				return *s_ins;
			}

			static T &instance()
			{
				if (s_ins)
				{
					return *s_ins;
				}
				std::call_once(s_once, [&]() {
					s_ins = new T();
				});
				return *s_ins;
			}

		private:
			static std::once_flag s_once;
			static T *s_ins;
		};

		template <typename T>
		std::once_flag Singleton<T>::s_once;
		template <typename T>
		T *Singleton<T>::s_ins = nullptr;

	} // namespace utils
} // namespace knet