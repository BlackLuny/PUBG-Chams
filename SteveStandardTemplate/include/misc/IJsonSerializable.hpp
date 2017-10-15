#pragma once

#include <vendor/nlohmann/json.hpp>

namespace SteveBase::Misc {
	template<class T>
	struct IJsonSerializable {
		using self_t = IJsonSerializable<T>;

		constexpr void to_json(nlohmann::json &j) const {
			auto self = const_cast<self_t *>(this);
			static_cast<T *>(self)->to_json(j);
		}

		constexpr void from_json(nlohmann::json &j) const {
			auto self = const_cast<self_t *>(this);
			static_cast<T *>(self)->from_json(j);
		}
	};

	template <class T>
	constexpr void to_json(nlohmann::json &j, const IJsonSerializable<T> &value) {
		value.to_json(j);
	}

	template <class T>
	constexpr void from_json(const nlohmann::json &j, IJsonSerializable<T> &value) {
		auto _j = const_cast<nlohmann::json &>(j);
		value.from_json(_j);
	}
}