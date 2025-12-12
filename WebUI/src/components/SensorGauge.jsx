export default function SensorGauge({ icon, label, value, unit, min, max, optimal }) {
  const percentage = ((value - min) / (max - min)) * 100
  const isOptimal = value >= optimal[0] && value <= optimal[1]
  const isWarning = value < optimal[0] - 5 || value > optimal[1] + 5
  const isCritical = value < min + 5 || value > max - 5

  let statusColor = 'bg-green-500'
  let statusText = 'Optimal'

  if (isCritical) {
    statusColor = 'bg-red-500'
    statusText = 'Critical'
  } else if (isWarning) {
    statusColor = 'bg-yellow-500'
    statusText = 'Warning'
  }

  return (
    <div className="card">
      <div className="flex items-start justify-between mb-4">
        <div className="flex items-center space-x-2">
          <div className="text-gray-600">{icon}</div>
          <h3 className="font-medium text-gray-700">{label}</h3>
        </div>
        <span className={`badge badge-${statusText.toLowerCase()}`}>{statusText}</span>
      </div>

      <div className="text-center mb-4">
        <div className="text-3xl font-bold text-gray-800">
          {value?.toFixed(1) || '--'}
          <span className="text-lg text-gray-600 ml-1">{unit}</span>
        </div>
      </div>

      <div className="relative pt-1">
        <div className="overflow-hidden h-2 text-xs flex rounded bg-gray-200">
          <div
            style={{ width: `${Math.min(Math.max(percentage, 0), 100)}%` }}
            className={`shadow-none flex flex-col text-center whitespace-nowrap text-white justify-center ${statusColor} transition-all duration-500`}
          ></div>
        </div>
        <div className="flex justify-between text-xs text-gray-600 mt-1">
          <span>{min}</span>
          <span>{max}</span>
        </div>
      </div>
    </div>
  )
}
