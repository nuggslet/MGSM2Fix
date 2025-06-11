#pragma once

#include "sqinvoker.h"

#include <type_traits>

template <Squirk Q>
class SQEmuTask : public SQInvoker<Q>
{
public:
	SQEmuTask() : SQInvoker<Q>(_SC("g_emu_task")) {}

	static SQInteger GetInfoInteger(const SQChar *key) {
		return SQEmuTask().Invoke<SQInteger>(__func__, key);
	}
	static void SetInfoInteger(const SQChar *key, SQInteger value) {
		return SQEmuTask().Invoke<void>(__func__, key, value);
	}

	static SQInteger GetRamValue(SQInteger width, SQInteger offset) {
		return SQEmuTask().Invoke<SQInteger>(__func__, width, offset);
	}
	static void SetRamValue(SQInteger width, SQInteger offset, SQInteger value) {
		return SQEmuTask().Invoke<void>(__func__, width, offset, value);
	}

	static void EntryCdRomPatch(SQInteger offset, Sqrat::Array<Q> & data) {
		return SQEmuTask().Invoke<void>(__func__, offset, data);
	}
	static void ReleaseCdRomPatch() {
		return SQEmuTask().Invoke<void>(__func__);
	}

	static SQBool GetSmoothing() {
		return SQEmuTask().Invoke<SQBool>(__func__);
	}
	static void SetSmoothing(SQBool enable) {
		return SQEmuTask().Invoke<void>(__func__, enable);
	}

	static SQBool GetScanline() {
		return SQEmuTask().Invoke<SQBool>(__func__);
	}
	static void SetScanline(SQBool enable) {
		return SQEmuTask().Invoke<void>(__func__, enable);
	}

	static SQInteger GetInputDirectionMerge() {
		return SQEmuTask().Invoke<SQInteger>(__func__);
	}
	static void SetInputDirectionMerge(SQInteger mode) {
		return SQEmuTask().Invoke<void>(__func__, mode);
	}

	static SQFloat GetInputDeadzone() {
		return SQEmuTask().Invoke<SQFloat>(__func__);
	}
	static void SetInputDeadzone(SQFloat value) {
		return SQEmuTask().Invoke<void>(__func__, value);
	}

	static void UpdateEmuScreen() {
		return SQEmuTask().Invoke<void>(__func__);
	}

	template <typename Source, typename Destination>
	static Destination RamCopy(Destination dst, Source src, std::size_t size) {
		Destination dest = dst;
		if constexpr (std::is_pointer_v<Source> && std::is_integral_v<Destination>) {
			const uint8_t * _src = reinterpret_cast<const uint8_t *>(src);
			while (size >= sizeof(uint32_t)) {
				const uint32_t * __src = reinterpret_cast<const uint32_t *>(_src);
				SetRamValue(sizeof(uint32_t) * CHAR_BIT, dst, *__src);
				size -= sizeof(uint32_t); _src += sizeof(uint32_t); dst += sizeof(uint32_t);
			}
			if (size >= sizeof(uint16_t)) {
				const uint16_t * __src = reinterpret_cast<const uint16_t *>(_src);
				SetRamValue(sizeof(uint16_t) * CHAR_BIT, dst, *__src);
				size -= sizeof(uint16_t); _src += sizeof(uint16_t); dst += sizeof(uint16_t);
			}
			if (size >= sizeof(uint8_t)) {
				const uint8_t * __src = reinterpret_cast<const uint8_t *>(_src);
				SetRamValue(sizeof(uint8_t) * CHAR_BIT, dst, *__src);
				size -= sizeof(uint8_t); _src += sizeof(uint8_t); dst += sizeof(uint8_t);
			}
			return dest;
		}
		else if constexpr (std::is_pointer_v<Destination> && std::is_integral_v<Source>) {
			uint8_t * _dst = reinterpret_cast<uint8_t *>(dst);
			while (size >= sizeof(uint32_t)) {
				uint32_t * __dst = reinterpret_cast<uint32_t *>(_dst);
				*__dst = GetRamValue(sizeof(uint32_t) * CHAR_BIT, src);
				size -= sizeof(uint32_t); src += sizeof(uint32_t); _dst += sizeof(uint32_t);
			}
			if (size >= sizeof(uint16_t)) {
				uint16_t * __dst = reinterpret_cast<uint16_t *>(_dst);
				*__dst = GetRamValue(sizeof(uint16_t) * CHAR_BIT, src);
				size -= sizeof(uint16_t); src += sizeof(uint16_t); _dst += sizeof(uint16_t);
			}
			if (size >= sizeof(uint8_t)) {
				uint8_t * __dst = reinterpret_cast<uint8_t *>(_dst);
				*__dst = GetRamValue(sizeof(uint8_t) * CHAR_BIT, src);
				size -= sizeof(uint8_t); src += sizeof(uint8_t); _dst += sizeof(uint8_t);
			}
			return dest;
		}
		else static_assert(false);
		return dest;
	}

};

template SQEmuTask<Squirk::Standard>;
template SQEmuTask<Squirk::AlignObject>;
template SQEmuTask<Squirk::StandardShared>;
template SQEmuTask<Squirk::AlignObjectShared>;
