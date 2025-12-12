import { NavLink } from 'react-router-dom'
import { Home, BarChart3, Bell, Settings } from 'lucide-react'

export default function Sidebar({ isOpen }) {
  const links = [
    { to: '/', icon: <Home />, label: 'Dashboard' },
    { to: '/analytics', icon: <BarChart3 />, label: 'Analytics' },
    { to: '/alerts', icon: <Bell />, label: 'Alerts' },
    { to: '/settings', icon: <Settings />, label: 'Settings' },
  ]

  return (
    <aside
      className={`fixed left-0 top-16 h-[calc(100vh-4rem)] w-64 bg-white border-r border-gray-200 transition-transform duration-300 ${
        isOpen ? 'translate-x-0' : '-translate-x-full'
      }`}
    >
      <nav className="p-4 space-y-2">
        {links.map((link) => (
          <NavLink
            key={link.to}
            to={link.to}
            className={({ isActive }) =>
              `flex items-center space-x-3 px-4 py-3 rounded-lg transition-colors ${
                isActive
                  ? 'bg-green-50 text-green-600'
                  : 'text-gray-700 hover:bg-gray-100'
              }`
            }
          >
            <span className="w-5 h-5">{link.icon}</span>
            <span className="font-medium">{link.label}</span>
          </NavLink>
        ))}
      </nav>
    </aside>
  )
}
