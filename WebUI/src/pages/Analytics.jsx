import HistoricalDataChart from '../components/HistoricalDataChart'

export default function Analytics() {
  return (
    <div className="space-y-6">
      <h1 className="text-3xl font-bold text-gray-800">Analytics</h1>

      <div className="card">
        <h2 className="text-xl font-semibold text-gray-700 mb-4">Historical Sensor Data</h2>
        <HistoricalDataChart greenhouseId="1" />
      </div>
    </div>
  )
}
