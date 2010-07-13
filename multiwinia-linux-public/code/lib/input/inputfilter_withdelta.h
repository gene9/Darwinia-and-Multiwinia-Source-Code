#ifndef INCLUDED_INPUTFILTER_WITHDELTA_H
#define INCLUDED_INPUTFILTER_WITHDELTA_H

#include "lib/auto_vector.h"
#include "lib/input/inputfilterspec.h"
#include "lib/input/inputfilter.h"


class InputFilterWithDelta : public InputFilter {

private:
	auto_vector<const InputFilterSpec> m_specs;
	auto_vector<InputDetails> m_oldDetails;
	auto_vector<InputDetails> m_details;

protected:
	void registerDeltaID( InputFilterSpec &spec );

	InputDetails const &getOldDetails( filterspec_id_t id ) const;
	InputDetails const &getOldDetails( InputFilterSpec const &spec ) const;

	InputDetails const &getDetails( filterspec_id_t id ) const;
	InputDetails const &getDetails( InputFilterSpec const &spec ) const;

	// Subclasses must implement this
	virtual void calcDetails( InputFilterSpec const &spec, InputDetails &details ) = 0;

	// Preferably override the first of these, unless you need to use more
	// filterspec information, in which case you should probably override both
	virtual int getDelta( filterspec_id_t id, InputDetails &delta ) const;
	virtual int getDelta( InputFilterSpec const &spec, InputDetails &delta ) const;

	// After this, the current details will be invalid
	void ageDetails();

public:
	virtual void Advance();

};


#endif // INCLUDED_INPUTFILTER_WITHDELTA_H
