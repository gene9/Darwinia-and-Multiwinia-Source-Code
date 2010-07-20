#ifndef INCLUDED_TARGETCURSOR_H
#define INCLUDED_TARGETCURSOR_H


class TargetCursor {

private:
	int m_screenCoords[3];
	int m_velocity[3];

public:
	TargetCursor();

	// The next frame is nigh
	void Advance();

	// Set the absolute cursor position, without causing a
	// velocity change
	void SetMousePos( int x, int y );

	// Simulate a cursor movement
	void MoveCursor( int x, int y );

	int X() const;

	int Y() const;

	int Z() const;

	int dX() const;

	int dY() const;

	int dZ() const;

};

extern TargetCursor *g_target;


#endif // INCLUDED_TARGETCURSOR_H
