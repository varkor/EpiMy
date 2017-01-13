#include <iostream>

#define POLYMORPHIC(class) \
	virtual ~class() = default;\
	class() = default;\
	class(const class&) = default;\
	class& operator=(const class& other) = default;

namespace EpiMy {
	namespace Adaptors {
		class Adaptor {
			public:
			POLYMORPHIC(Adaptor)
			
			virtual void execute(std::string string) = 0;
		};
		
		class Epilog: public Adaptor {
			public:
			virtual void execute(std::string string) override;
		};
		
		class MysoreScript: public Adaptor {
			public:
			virtual void execute(std::string string) override;
		};
	}
}
