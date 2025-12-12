import { useState, useEffect } from 'react'
import { getFirestore, collection, query, orderBy, limit, onSnapshot } from 'firebase/firestore'
import { Line } from 'react-chartjs-2'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend)

export default function SensorChart({ greenhouseId }) {
  const [chartData, setChartData] = useState(null)
  const db = getFirestore()

  useEffect(() => {
    const q = query(
      collection(db, 'greenhouses', greenhouseId, 'sensors'),
      orderBy('timestamp', 'desc'),
      limit(20)
    )

    const unsubscribe = onSnapshot(q, (snapshot) => {
      const data = []
      snapshot.forEach((doc) => {
        data.push(doc.data())
      })

      // Reverse to show oldest first
      data.reverse()

      const labels = data.map((d) => new Date(d.timestamp).toLocaleTimeString())
      const temps = data.map((d) => d.airTemp)
      const humidity = data.map((d) => d.airHumidity)

      setChartData({
        labels,
        datasets: [
          {
            label: 'Temperature (°C)',
            data: temps,
            borderColor: 'rgb(239, 68, 68)',
            backgroundColor: 'rgba(239, 68, 68, 0.1)',
            yAxisID: 'y',
          },
          {
            label: 'Humidity (%)',
            data: humidity,
            borderColor: 'rgb(59, 130, 246)',
            backgroundColor: 'rgba(59, 130, 246, 0.1)',
            yAxisID: 'y1',
          },
        ],
      })
    })

    return () => unsubscribe()
  }, [db, greenhouseId])

  const options = {
    responsive: true,
    interaction: {
      mode: 'index',
      intersect: false,
    },
    plugins: {
      legend: {
        position: 'top',
      },
    },
    scales: {
      y: {
        type: 'linear',
        display: true,
        position: 'left',
      },
      y1: {
        type: 'linear',
        display: true,
        position: 'right',
        grid: {
          drawOnChartArea: false,
        },
      },
    },
  }

  if (!chartData) {
    return <div className="text-center text-gray-600">Loading chart...</div>
  }

  return <Line data={chartData} options={options} />
}
