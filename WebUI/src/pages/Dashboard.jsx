import { useState, useEffect } from 'react'
import { getFirestore, collection, query, orderBy, limit, onSnapshot } from 'firebase/firestore'
import { getApp } from 'firebase/app'
import { greenhouseId } from '../config'
import { Thermometer, Droplets, Wind, Activity } from 'lucide-react'

import SensorGauge from '../components/SensorGauge'
import SensorChart from '../components/SensorChart'
import AlertList from '../components/AlertList'

export default function Dashboard() {
  const [sensorData, setSensorData] = useState(null)
  const [loading, setLoading] = useState(true)
  const [lastUpdate, setLastUpdate] = useState(null)

  const app = getApp()
  const db = getFirestore(app)

  useEffect(() => {
    // Real-time listener for latest sensor data
    const q = query(
      collection(db, 'greenhouses', greenhouseId, 'sensors'),
      orderBy('timestamp', 'desc'),
      limit(1)
    )

    const unsubscribe = onSnapshot(q, (snapshot) => {
      if (!snapshot.empty) {
        const data = snapshot.docs[0].data()
        setSensorData(data)
        setLastUpdate(new Date(data.timestamp))
      }
      setLoading(false)
    })

    return () => unsubscribe()
  }, [db])

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="spinner"></div>
      </div>
    )
  }

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h1 className="text-3xl font-bold text-gray-800">Dashboard</h1>
        {lastUpdate && (
          <div className="text-sm text-gray-600">
            Last update: {lastUpdate.toLocaleTimeString()}
          </div>
        )}
      </div>

      {/* Sensor Gauges */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <SensorGauge
          icon={<Thermometer />}
          label="Air Temperature"
          value={sensorData?.airTemp}
          unit="°C"
          min={0}
          max={40}
          optimal={[18, 24]}
        />
        <SensorGauge
          icon={<Droplets />}
          label="Humidity"
          value={sensorData?.airHumidity}
          unit="%"
          min={0}
          max={100}
          optimal={[40, 80]}
        />
        <SensorGauge
          icon={<Wind />}
          label="CO₂"
          value={sensorData?.co2}
          unit="ppm"
          min={400}
          max={1500}
          optimal={[600, 1200]}
        />
        <SensorGauge
          icon={<Activity />}
          label="Soil Moisture"
          value={sensorData?.vwc}
          unit="%"
          min={0}
          max={100}
          optimal={[20, 60]}
        />
      </div>

      {/* Charts and Alerts */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2">
          <div className="card">
            <h2 className="text-xl font-semibold mb-4">Recent Trends</h2>
            <SensorChart greenhouseId={greenhouseId} />
          </div>
        </div>

        <div className="lg:col-span-1">
          <div className="card">
            <h2 className="text-xl font-semibold mb-4">Active Alerts</h2>
            <AlertList greenhouseId={greenhouseId} limit={5} />
          </div>
        </div>
      </div>
    </div>
  )
}
