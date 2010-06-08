// Inlines

inline const Vector3 &NetworkUpdate::GetWorldPos() const
{
	return m_teamControls.m_mousePos;
}

inline Vector3 &NetworkUpdate::GetWorldPos() 
{
	return m_teamControls.m_mousePos;
}
