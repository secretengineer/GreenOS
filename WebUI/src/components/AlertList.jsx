import { useState, useEffect } from 'react'
import { getFirestore, collection, query, where, orderBy, limit, onSnapshot } from 'firebase/firestore'
import { AlertCircle, AlertTriangle, Info } from 'lucide-react'

export default function AlertList({ greenhouseId, limit: maxLimit = 10 }) {
  const [alerts, setAlerts] = useState([])
  const db = getFirestore()

  useEffect(() => {
    const q = query(
      collection(db, 'greenhouses', greenhouseId, 'alerts'),
      where('acknowledged', '==', false),
      orderBy('timestamp', 'desc'),
      limit(maxLimit)
    )

    const unsubscribe = onSnapshot(q, (snapshot) => {
      const alertData = []
      snapshot.forEach((doc) => {
        alertData.push({ id: doc.id, ...doc.data() })
      })
      setAlerts(alertData)
    })

    return () => unsubscribe()
  }, [db, greenhouseId, maxLimit])

  const getAlertIcon = (severity) => {
    switch (severity) {
      case 'critical':
        return <AlertCircle className="w-5 h-5 text-red-600" />
      case 'high':
        return <AlertTriangle className="w-5 h-5 text-yellow-600" />
      default:
        return <Info className="w-5 h-5 text-blue-600" />
    }
  }

  const getSeverityBadge = (severity) => {
    const classes = {
      critical: 'badge-critical',
      high: 'badge-high',
      medium: 'badge-medium',
      low: 'badge-low',
    }
    return classes[severity] || classes.low
  }

  if (alerts.length === 0) {
    return (
      <div className="text-center py-8 text-gray-500">
        <p>No active alerts</p>
        <p className="text-sm">All systems operating normally</p>
      </div>
    )
  }

  return (
    <div className="space-y-3">
      {alerts.map((alert) => (
        <div
          key={alert.id}
          className="flex items-start space-x-3 p-3 bg-gray-50 rounded-lg hover:bg-gray-100 transition-colors"
        >
          <div className="flex-shrink-0 mt-1">{getAlertIcon(alert.severity)}</div>
          <div className="flex-1 min-w-0">
            <div className="flex items-center justify-between mb-1">
              <span className={`badge ${getSeverityBadge(alert.severity)}`}>
                {alert.type}
              </span>
              <span className="text-xs text-gray-500">
                {new Date(alert.timestamp?.toDate()).toLocaleTimeString()}
              </span>
            </div>
            <p className="text-sm text-gray-700">{alert.message}</p>
          </div>
        </div>
      ))}
    </div>
  )
}
