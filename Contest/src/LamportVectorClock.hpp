#ifndef CONTEST_LAMPORTCLOCK_HPP
#define CONTEST_LAMPORTCLOCK_HPP

#include <vector>
#include <mutex>
#include <memory>

class LamportVectorClock
{
public:

	LamportVectorClock(const int size, const int id);

	void update(const std::vector<int> otherVector);

	std::vector<int> getCurrent();

	void update();

private:

	std::vector<int> _clockVector;
	std::mutex _clockMutex;

	int _id;
};

std::ostringstream& operator<<(std::ostringstream& os, LamportVectorClock& obj);

typedef std::shared_ptr<LamportVectorClock> LamportVectorClockPtr;

#endif //CONTEST_LAMPORTCLOCK_HPP
