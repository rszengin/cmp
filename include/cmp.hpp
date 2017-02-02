//============================================================================
// Name        : chain.hpp
// Author      : Rahman Salim Zengin
// Version     :
// Copyright   : rsz@gufatek.com
// Description : Chained comparison implementation
//============================================================================

#ifndef CMP_HPP_
#define CMP_HPP_

#include <type_traits>
#include <cassert>
#include <tuple>
#include <functional>

namespace cmp {

	template<std::size_t N> struct Element {};

	template<
		std::size_t Index, // Chain node index number
		class Parent, // Type of the parent node which generated this
		class OpType, // Preceding operator class type : std::less<> OR std::less_equal<>
		typename T // Type of the comparison argument stored by this node 
	>
	class Node : public Parent {
	public:
		using this_type = Node<Index, Parent, OpType, T>;
		using parent = Parent;
		const T& elem;

		Node(const Node& node) = default;
		Node(Parent&& pclass, T&& elem) : parent(pclass), elem(elem) {}
		Node(T&& elem) : elem(elem) {}

		template<typename RType>
		auto operator<(RType&& rhs) {
			return Node<Index + 1, this_type, std::less<>, RType>(std::move(*this), std::forward<RType>(rhs));
		}

		template<typename RType>
		auto operator<=(RType&& rhs) {
			return Node<Index + 1, this_type, std::less_equal<>, RType>(std::move(*this), std::forward<RType>(rhs));
		}

		static constexpr std::size_t size() { return Index + 1; }

		using parent::get;
		const T& get(Element<Index>) const { return elem; }

		using parent::op;
		OpType op(Element<Index>) const { return{}; }

		template<std::size_t Inx>
		decltype(auto) get() { return (get(Element<Inx>{})); }

		template<std::size_t Inx>
		decltype(auto) op() { return op(Element<Inx>{}); }

		/* The range is including Final */
		template<std::size_t Current, std::size_t Step, std::size_t Final, typename Func>
		std::enable_if_t<(Current <= Final)> for_helper(Func func) {
			func(get<Current>());
			for_helper<Current + Step, Step, Final>(func);
		};
		template<std::size_t Current, std::size_t Step, std::size_t Final, typename Func>
		std::enable_if_t<(Current > Final)> for_helper(Func func) {};

		template<typename Func>
		void foreach(Func func) {
			for_helper<0, 1, size() - 1>(func);
		}


		template<std::size_t LIndex, std::size_t RIndex>
		std::enable_if_t<(LIndex < RIndex), bool> check_all_less_equal() {
			// op<LIndex + 1>() is the succeeding operator
			if (std::is_same<decltype(op<LIndex + 1>()), std::less_equal<>>::value) {
				return check_all_less_equal<LIndex + 1, RIndex>();
			}
			return false;
		}
		template<std::size_t LIndex, std::size_t RIndex>
		std::enable_if_t<(LIndex >= RIndex), bool> check_all_less_equal() {
			return true;
		}

		template<std::size_t LIndex, std::size_t RIndex>
		bool compare() {
			if (check_all_less_equal<LIndex, RIndex>()) {
				return std::less_equal<>{}(get<LIndex>(), get<RIndex>());
			}
			else {
				return std::less<>{}(get<LIndex>(), get<RIndex>());
			}
		}

		template<std::size_t LIndex, std::size_t Dist>
		std::enable_if_t<(LIndex + Dist < size()), bool> eval_for_dist() {
			return compare<LIndex, LIndex + Dist>() && eval_for_dist<LIndex + 1, Dist>();
		}
		template<std::size_t LIndex, std::size_t Dist>
		std::enable_if_t<(LIndex + Dist >= size()), bool> eval_for_dist() {
			return true;
		}

		template<std::size_t Dist = 1>
		std::enable_if_t<(Dist < size()), bool> evaluate() {
			return eval_for_dist<0, Dist>() && evaluate<Dist + 1>();
		}
		template<std::size_t Dist>
		std::enable_if_t<(Dist >= size()), bool> evaluate() {
			return true;
		}

		operator bool() {
			return evaluate();
		}

	};

	struct Root_node { 
		void get() {}; // This function is required by "using parent::get"
		void op() {}; // This function is required by "using parent::op"
	};

	class Chain_root {
	public:
		template<typename RType>
		auto operator<<(RType&& rhs) {
			return Node<0, cmp::Root_node, void, RType>(std::forward<RType>(rhs));
		}
	};
	
	Chain_root chain_root{};
	

	//##########################################################################3

	template<typename T, typename... Types>
	class Expr_store_base : public Expr_store_base<Types...> {
		using super = Expr_store_base<Types...>;
		const T& elem;
	public:
		Expr_store_base(const T& elem, const Types&... others)
			: super(others...), elem(elem) {}

		using super::get;
		const T& get(Element<sizeof...(Types)>) { return elem; }
	};

	template<typename T>
	class Expr_store_base<T> {
		const T& elem;
	public:
		Expr_store_base(const T& elem) : elem(elem) {}

		const T& get(Element<0>) { return elem; }
	};

	template<typename... Types>
	class Expr_store : Expr_store_base<Types...> {
		using super = Expr_store_base<Types...>;
	public:
		Expr_store(const Types&... args) : super(args...) {}

		template<std::size_t Index>
		decltype(auto) get() { return (super::get(Element<Index>{})); }
	};




	// Helper type for controlling of the chain order
	struct Order {
		struct Ascending;
		struct Descending;
	};


	struct Operation {
		struct Base {};
		struct ChainBegin {};
		struct ChainEnd {};
		struct LowerThan {};
		struct LowerThanEqual {};
		struct GreaterThan {};
		struct GreaterThanEqual {};
	};


	// Operation::Base
	template<typename RType, typename OperType>
	struct Reserve {
		const RType& rhs_;
		typedef RType r_type;

		Reserve() : rhs_() {}
		Reserve(const RType& rhs) : rhs_(rhs) {}
		Reserve(const Reserve& reserve) : rhs_(reserve.rhs_) {}
	};

	template<typename RType>
	class Reserve<RType, Operation::ChainBegin> :
		public Reserve<RType, Operation::Base> {
		using super = Reserve<RType, Operation::Base>;
	public:
		typedef typename Operation::ChainBegin oper_type;

		Reserve() : super() {}
		Reserve(const RType& rhs) : super(rhs) {}
		Reserve(const Reserve& reserve) : super(reserve) {}
	};

	template<typename RType>
	class Reserve<RType, Operation::LowerThan> :
		public Reserve<RType, Operation::Base> {
		using super = Reserve<RType, Operation::Base>;
	public:
		typedef typename Operation::LowerThan oper_type;

		Reserve() : super() {}
		Reserve(const RType& rhs) : super(rhs) {}
		Reserve(const Reserve& reserve) : super(reserve) {}
	};

	template<typename LType, typename RType>
	bool compare(const LType& lhs, Reserve<RType, Operation::LowerThan>& rhs) {
		return lhs < rhs.rhs_;
	}

	template<typename LType, typename RType>
	bool compare(const LType& lhs, Reserve<RType, Operation::LowerThanEqual>& rhs) {
		return lhs <= rhs.rhs_;
	}

	template<std::size_t OuterInx, std::size_t InnerInx, std::size_t EndInx, typename... Types>
	class Implement {
	public:
		static bool comparison(const std::tuple<Types...>& tuple) {
			auto first = std::get<OuterInx>(tuple);
			auto second = std::get<InnerInx>(tuple);
			return compare(first.rhs_, second) &&
				Implement<OuterInx, InnerInx + 1, EndInx, Types...>::comparison(tuple);
		}
	};

	template<std::size_t OuterInx, std::size_t EndInx, typename... Types>
	class Implement<OuterInx, EndInx, EndInx, Types...> {
	public:
		static bool comparison(const std::tuple<Types...>& tuple) {
			auto first = std::get<OuterInx>(tuple);
			auto second = std::get<EndInx>(tuple);
			return compare(first.rhs_, second) &&
				Implement<OuterInx + 1, OuterInx + 2, EndInx, Types...>::comparison(tuple);
		}
	};

	template<std::size_t InnerInx, std::size_t EndInx, typename... Types>
	class Implement<EndInx, InnerInx, EndInx, Types...> {
	public:
		static bool comparison(const std::tuple<Types...>& tuple) {
			return true;
		}
	};


	/*
	 * "Conductor" object preserves information before and transfers it to the following.
	 * Operations should follow chain order.
	 * Ascending to ascending or descending to descending.
	 * Breaking the order will result in a compile time error.
	 * Mixes comparison order is not allowed.
	 * Internal state is constant after construction.
	 */
	template<typename ChainOrder, std::size_t OperCnt, typename... Previous>
	class Conductor {
	public:
		std::tuple<Previous...> previous_;

		template<typename... Types>
		Conductor(std::tuple<Types...> previous) : previous_(previous) {}

		/*
		 * These operators are only allowed for an "Ascending" ordered comparison chain.
		 * Otherwise will result in a compile time error.
		 */
		 //	template<typename RType>
		 //	Conductor<Order::Ascending, RType> operator <(RType& rhs) const {
		 //		static_assert(std::is_same<ChainOrder, Order::Ascending>::value,
		 //				"Ambiguously ordered comparison chain");
		 //		return Conductor<RType, Order::Ascending>(rhs, result_ && (lhs_ < rhs));
		 //	}
		template<typename RType>
		auto operator <(const RType& rhs) const {
			using reserve_type = Reserve<RType, Operation::LowerThan>;
			static_assert(std::is_same<ChainOrder, Order::Ascending>::value,
				"Ambiguously ordered comparison chain");
			return Conductor<Order::Ascending, OperCnt + 1, Previous..., reserve_type>(
				std::tuple_cat(previous_, std::make_tuple(reserve_type(rhs))));
		}
		//	template<typename RType>
		//	Conductor<RType, Order::Ascending, KnownType> operator <=(RType& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Ascending>::value,
		//				"Ambiguously ordered comparison chain");
		//		return Conductor<RType, Order::Ascending, KnownType>(rhs, result_ && (lhs_ <= rhs), known_value_);
		//	}
		//	template<typename RType>
		//	Conductor<RType, Order::Ascending, RType> operator <=(RType&& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Ascending>::value,
		//				"Ambiguously ordered comparison chain");
		//		assert(("Always false", !std::is_same<KnownType, Unknown_t>::value && (known_value_ > rhs)));
		//		return Conductor<RType, Order::Ascending, RType>(rhs, result_ && (lhs_ <= rhs), rhs);
		//	}

		//	/*
		//	 * These operators are only allowed for an "Descending" ordered comparison chain.
		//	 * Otherwise will result in a compile time error.
		//	 */
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, KnownType> operator >(RType& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Descending>::value,
		//				"Ambiguously ordered comparison chain");
		//		return Conductor<RType, Order::Descending, KnownType>(rhs, result_ && (lhs_ > rhs), known_value_);
		//	}
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, RType> operator >(RType&& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Descending>::value,
		//				"Ambiguously ordered comparison chain");
		//		assert(("Always false", !std::is_same<KnownType, Unknown_t>::value && (known_value_ <= rhs)));
		//		return Conductor<RType, Order::Descending, RType>(rhs, result_ && (lhs_ > rhs), rhs);
		//	}
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, KnownType> operator>=(RType& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Descending>::value,
		//				"Ambiguously ordered comparison chain");
		//		return Conductor<RType, Order::Descending, KnownType>(rhs, result_ && (lhs_>= rhs), known_value_);
		//	}
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, RType> operator>=(RType&& rhs) const {
		//		static_assert(std::is_same<ChainOrder, Order::Descending>::value,
		//				"Ambiguously ordered comparison chain");
		//		assert(("Always false", !std::is_same<KnownType, Unknown_t>::value && (known_value_ < rhs)));
		//		return Conductor<RType, Order::Descending, RType>(rhs, result_ && (lhs_>= rhs), rhs);
		//	}


		inline operator bool() const {
			return Implement<0, 1, sizeof...(Previous)-1, Previous...>::comparison(previous_);
		}
	};


	/*
	 *	"Initiator" object starts comparison chaining and generates "Conductor" object.
	 *	First operator after the "Initiator" object determines ordering of the chain.
	 *	Starting with "<" forces ascending order.
	 *	For ascending, only "<" and "<=" is allowed.
	 *	Starting with ">" forces descending order.
	 *	For descending, only ">" and ">=" is allowed.
	 */
	class Initiator {

	public:
		//	template<typename RType>
		//	Conductor<RType, Order::Ascending> operator <<(RType& rhs) const {
		//		return Conductor<RType, Order::Ascending>(rhs, true);
		//	}
		template<typename RType>
		auto operator <<(const RType& rhs) const {
			using reserve_type = Reserve<RType, Operation::ChainBegin>;
			return Conductor<Order::Ascending, 1, reserve_type>(
				std::make_tuple(reserve_type(rhs)));
		}
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, Unknown_t> operator >>(RType& rhs) const {
		//		return Conductor<RType, Order::Descending, Unknown_t>(rhs, true, Unknown_t {});
		//	}
		//	template<typename RType>
		//	Conductor<RType, Order::Descending, RType> operator >>(RType&& rhs) const {
		//		return Conductor<RType, Order::Descending, RType>(rhs, true, rhs);
		//	}
	};

	/*
	 * "chain" is the default "Initiator" instance. It doesn't have internal state.
	 * It is defined for ease and comfort. Usage of "Initiator {}" in place of "chain"
	 * is also possible.
	 */
	Initiator chain{};

} /* namespace cmp */



#endif /* CMP_HPP_ */
